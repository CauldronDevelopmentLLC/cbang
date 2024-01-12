/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#include "PowerManagement.h"

#include <cbang/config.h>
#include <cbang/Exception.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/Catch.h>
#include <cbang/time/Time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#elif defined(__APPLE__)
#include <cbang/os/MacOSString.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#elif defined(HAVE_SYSTEMD)
#include <systemd/sd-bus.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <cstring>

using namespace cb;
using namespace std;


namespace {
#if defined(_WIN32)
  void win32AllowSleep(bool system) {
    SetThreadExecutionState(
      ES_CONTINUOUS |
      (system ? 0 : (ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED)));
  }


  unsigned win32GetIdleSeconds() {
    LASTINPUTINFO lif;
    lif.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lif);

    return (GetTickCount() - lif.dwTime) / 1000; // Convert from ms.
  }


  void win32GetBatteryInfo(bool &onBattery, bool &hasBattery) {
    onBattery = hasBattery = false;

    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
      onBattery = status.ACLineStatus == 0;
      hasBattery = (status.BatteryFlag & 128) == 0;
    }
  }


#elif defined(__APPLE__)
  void macOSAllowSleep(bool allow, IOPMAssertionID &aid) {
    if (!allow) {
      MacOSString name(
        SystemUtilities::basename(SystemUtilities::getExecutablePath()));

      if (IOPMAssertionCreateWithName(
            kIOPMAssertionTypeNoIdleSleep, kIOPMAssertionLevelOn, name, &aid) !=
          kIOReturnSuccess)
        aid = 0;

    } else if (aid) {
      IOPMAssertionRelease(aid);
      aid = 0;
    }
  }


  unsigned macOSGetIdleSeconds() {
    unsigned idleSeconds = 0;
    io_iterator_t iter = 0;

    if (IOServiceGetMatchingServices(
          kIOMasterPortDefault,
          IOServiceMatching("IOHIDSystem"), &iter) == KERN_SUCCESS) {

      io_registry_entry_t entry = IOIteratorNext(iter);
      if (entry)  {
        CFMutableDictionaryRef dict = 0;
        if (IORegistryEntryCreateCFProperties(
              entry, &dict, kCFAllocatorDefault, 0) == KERN_SUCCESS) {
          CFNumberRef obj =
            (CFNumberRef)CFDictionaryGetValue(dict, CFSTR("HIDIdleTime"));

          if (obj) {
            int64_t nanoseconds = 0;
            if (CFNumberGetValue(obj, kCFNumberSInt64Type, &nanoseconds))
              idleSeconds = (unsigned)(nanoseconds / 1000000000); // from ns
          }

          CFRelease(dict);
        }

        IOObjectRelease(entry);
      }

      IOObjectRelease(iter);
    }

    return idleSeconds;
  }


  void macOSGetBatteryInfo(bool &onBattery, bool &hasBattery) {
    onBattery = hasBattery = false;

    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    if (!info) return;

    CFArrayRef list = IOPSCopyPowerSourcesList(info);

    if (list) {
      CFIndex count = CFArrayGetCount(list);
      if (count > 0) hasBattery = true;

      for (CFIndex i = 0; i < count; i++) {
        CFTypeRef item = CFArrayGetValueAtIndex(list, i);
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(info, item);
        CFTypeRef value =
          CFDictionaryGetValue(battery, CFSTR(kIOPSPowerSourceStateKey));

        if (!CFEqual(value, CFSTR(kIOPSACPowerValue))) {
          onBattery = true;
          break;
        }
      }

      CFRelease(list);
    }

    CFRelease(info);
  }


#else

#if defined(HAVE_SYSTEMD)
  void systemdAllowSleep(sd_bus *bus, int &fd, bool allow) {
    if (!bus) return;

    if (0 < fd) close(fd);
    fd = -1;

    if (!allow) {
      sd_bus_message *reply = 0;
      sd_bus_error    error = SD_BUS_ERROR_NULL;
      string who =
        SystemUtilities::basename(SystemUtilities::getExecutablePath());

      if (sd_bus_call_method(
            bus, "org.freedesktop.login1", "/org/freedesktop/login1",
            "org.freedesktop.login1.Manager", "Inhibit", &error, &reply, "ssss",
            "sleep", who.c_str(), "Prevent system sleep", "block") < 0)
        THROW("Failed to prevent sleep: " << error.message);

      int tmpFD = -1;
      sd_bus_message_read_basic(reply, SD_BUS_TYPE_UNIX_FD, &tmpFD);

      if (0 < tmpFD) fd = fcntl(tmpFD, F_DUPFD_CLOEXEC, 3);

      sd_bus_error_free(&error);
      sd_bus_message_unref(reply);
    }
  }


  unsigned systemdGetIdleSeconds(sd_bus *bus) {
    int idle = 0;

    // logind API does not exactly match Windows and macOS ones,
    // if IdleHint is true, assume enough time without input has passed
    if (!bus || sd_bus_get_property_trivial(
          bus, "org.freedesktop.login1", "/org/freedesktop/login1/seat/seat0",
          "org.freedesktop.login1.Seat", "IdleHint", 0, 'b', &idle) < 0)
      return 0;

    return idle ? -1 : 0;
  }
#endif // HAVE_SYSTEMD


  void linuxGetBatteryInfo(bool &onBattery, bool &hasBattery) {
    onBattery = hasBattery = false;

    const char *sysBase = "/sys/class/power_supply";
    const char *procBase = "/proc/acpi/ac_adapter";

    bool useSys  = SystemUtilities::exists(sysBase);
    bool useProc = !useSys && SystemUtilities::exists(procBase);

    if (!useSys && !useProc) return;

    string base = useSys ? sysBase : procBase;

    // Has battery
    const char *batNames[] = {"/BAT", "/BAT0", "/BAT1", 0};
    for (int i = 0; !hasBattery && batNames[i]; i++)
      hasBattery = SystemUtilities::exists(base + batNames[i]);

    // On battery
    if (hasBattery) {
      if (useSys) {
        const char *acNames[] = { "/AC0/online", "/AC/online", 0};

        for (int i = 0; acNames[i]; i++) {
          string path = base + acNames[i];

          if (SystemUtilities::exists(path))
            onBattery = String::trim(SystemUtilities::read(path)) == "0";
        }

      } else if (useProc) {
        string path = base + "/AC0/state";

        if (SystemUtilities::exists(path)) {
          vector<string> tokens;
          String::tokenize(SystemUtilities::read(path), tokens);

          if (tokens.size() == 2 && tokens[0] == "state")
            onBattery = tokens[1] != "on-line";
        }
      }
    }
  }

#endif
}



struct PowerManagement::private_t {
#if defined(__APPLE__)
  IOPMAssertionID systemAssertionID;

#elif defined(HAVE_SYSTEMD)
  sd_bus *bus;
  int sleepFD;
#endif
};


PowerManagement::PowerManagement(Inaccessible) :
  lastBatteryUpdate(0), systemOnBattery(false), systemHasBattery(false),
  lastIdleSecondsUpdate(0), idleSeconds(0), systemSleepAllowed(true),
  pri(new private_t) {
  memset(pri, 0, sizeof(private_t));

#if defined(HAVE_SYSTEMD)
  sd_bus_open_system(&pri->bus);
#endif
}


PowerManagement::~PowerManagement() {
  if (!pri) return;

  allowSystemSleep(true);

#if defined(HAVE_SYSTEMD)
  pri->bus = sd_bus_flush_close_unref(pri->bus);
#endif

  delete pri;
}


bool PowerManagement::onBattery() {
  updateBatteryInfo();
  return systemOnBattery;
}


bool PowerManagement::hasBattery() {
  updateBatteryInfo();
  return systemHasBattery;
}


unsigned PowerManagement::getIdleSeconds() {
  updateIdleSeconds();
  return idleSeconds;
}


void PowerManagement::allowSystemSleep(bool x) {
  if (systemSleepAllowed == x) return;
  systemSleepAllowed = x;

  LOG_DEBUG(5, (x ? "Allowing" : "Disabling") << " system sleep");

#if defined(_WIN32)
  win32AllowSleep(systemSleepAllowed);
#elif defined(__APPLE__)
  macOSAllowSleep(systemSleepAllowed, pri->systemAssertionID);
#elif defined(HAVE_SYSTEMD)
  systemdAllowSleep(pri->bus, pri->sleepFD, systemSleepAllowed);
#endif
}


void PowerManagement::updateIdleSeconds() {
  // Avoid checking this too often
  if (Time::now() <= lastIdleSecondsUpdate) return;
  lastIdleSecondsUpdate = Time::now();

#if defined(_WIN32)
  idleSeconds = win32GetIdleSeconds();
#elif defined(__APPLE__)
  idleSeconds = macOSGetIdleSeconds();
#elif defined(HAVE_SYSTEMD)
  idleSeconds = systemdGetIdleSeconds(pri->bus);
#endif // HAVE_SYSTEMD
}


void PowerManagement::updateBatteryInfo() {
  // Avoid checking this too often
  if (Time::now() <= lastBatteryUpdate) return;
  lastBatteryUpdate = Time::now();

  try {
#if defined(_WIN32)
    win32GetBatteryInfo(systemOnBattery, systemHasBattery);
#elif defined(__APPLE__)
    macOSGetBatteryInfo(systemOnBattery, systemHasBattery);
#else
    linuxGetBatteryInfo(systemOnBattery, systemHasBattery);
#endif
  } CBANG_CATCH_ERROR;
}
