/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#elif defined(HAVE_SYSTEMD)
#include <systemd/sd-bus.h>
#endif

#include <cstring>

using namespace cb;
using namespace std;


struct PowerManagement::private_t {
#if defined(__APPLE__)
  IOPMAssertionID displayAssertionID;
  IOPMAssertionID systemAssertionID;

#elif defined(HAVE_SYSTEMD)
  sd_bus *bus;
#endif
};


PowerManagement::PowerManagement(Inaccessible) :
  lastBatteryUpdate(0), systemOnBattery(false), systemHasBattery(false),
  lastIdleSecondsUpdate(0), idleSeconds(0), systemSleepAllowed(true),
  displaySleepAllowed(true), pri(new private_t) {
  memset(pri, 0, sizeof(private_t));

#if defined(HAVE_SYSTEMD)
  sd_bus_open_system(&pri->bus);
#endif
}


PowerManagement::~PowerManagement() {
  allowSystemSleep(true);
  allowDisplaySleep(true);

#if defined(HAVE_SYSTEMD)
  pri->bus = sd_bus_flush_close_unref(pri->bus);
#endif
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

#if defined(_WIN32)
  systemSleepAllowed = x;
  SetThreadExecutionState(ES_CONTINUOUS |
                          (systemSleepAllowed ? 0 :
                           (ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED)) |
                          (displaySleepAllowed ? 0 : ES_DISPLAY_REQUIRED));

#elif defined(__APPLE__)
  IOPMAssertionID &assertionID = pri->systemAssertionID;

  if (!x) {
    if (IOPMAssertionCreateWithName(
          kIOPMAssertionTypeNoIdleSleep, kIOPMAssertionLevelOn,
          CFSTR("FAHClient"), &assertionID) == kIOReturnSuccess)
      systemSleepAllowed = false;

    else assertionID = 0;

  } else if (assertionID) {
    IOPMAssertionRelease(assertionID);
    assertionID = 0;
    systemSleepAllowed = true;
  }
#endif
}


void PowerManagement::allowDisplaySleep(bool x) {
  if (displaySleepAllowed == x) return;

#if defined(_WIN32)
  displaySleepAllowed = x;
  SetThreadExecutionState(
    ES_CONTINUOUS |
    (systemSleepAllowed ? 0 : (ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED)) |
    (displaySleepAllowed ? 0 : ES_DISPLAY_REQUIRED));

#elif defined(__APPLE__)
  IOPMAssertionID &assertionID = pri->displayAssertionID;

  if (!x) {
    if (IOPMAssertionCreateWithName(
          kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn,
          CFSTR("FAHClient"), &assertionID) == kIOReturnSuccess)
      displaySleepAllowed = false;

    else assertionID = 0;

  } else if (assertionID) {
    IOPMAssertionRelease(assertionID);
    assertionID = 0;
    displaySleepAllowed = false;
  }
#endif
}


void PowerManagement::updateIdleSeconds() {
  // Avoid checking this too often
  if (Time::now() <= lastIdleSecondsUpdate) return;
  lastIdleSecondsUpdate = Time::now();

  idleSeconds = 0;

#if defined(_WIN32)
  LASTINPUTINFO lif;
  lif.cbSize = sizeof(LASTINPUTINFO);
  GetLastInputInfo(&lif);

  idleSeconds = (GetTickCount() - lif.dwTime) / 1000; // Convert from ms.

#elif defined(__APPLE__)
  io_iterator_t iter = 0;

  if (IOServiceGetMatchingServices(
        kIOMasterPortDefault,
        IOServiceMatching("IOHIDSystem"), &iter) == KERN_SUCCESS) {

    io_registry_entry_t entry = IOIteratorNext(iter);
    if (entry)  {
      CFMutableDictionaryRef dict = NULL;
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

#elif defined(HAVE_SYSTEMD)
  int r, idle;

  if (!pri->bus) return;

  r = sd_bus_get_property_trivial(
    pri->bus, "org.freedesktop.login1", "/org/freedesktop/login1/seat/seat0",
    "org.freedesktop.login1.Seat", "IdleHint", 0, 'b', &idle);

  if (0 <= r && idle)
    // logind API does not exactly match Windows and macOS ones,
    // if IdleHint is true, assume enough time without input has passed
    idleSeconds = -1;
#endif // HAVE_SYSTEMD
}


void PowerManagement::updateBatteryInfo() {
  // Avoid checking this too often
  if (Time::now() <= lastBatteryUpdate) return;
  lastBatteryUpdate = Time::now();

  systemOnBattery = systemHasBattery  = false;

#if defined(_WIN32)
  SYSTEM_POWER_STATUS status;

  if (GetSystemPowerStatus(&status)) {
    systemOnBattery = status.ACLineStatus == 0;
    systemHasBattery = (status.BatteryFlag & 128) == 0;
  }

#elif defined(__APPLE__)
  CFTypeRef info = IOPSCopyPowerSourcesInfo();
  if (info) {
    CFArrayRef list = IOPSCopyPowerSourcesList(info);
    if (list) {
      CFIndex count = CFArrayGetCount(list);
      if (count > 0) systemHasBattery = true;
      for (CFIndex i=0; i < count; i++) {
        CFTypeRef item = CFArrayGetValueAtIndex(list, i);
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(info, item);
        CFTypeRef value =
          CFDictionaryGetValue(battery, CFSTR(kIOPSPowerSourceStateKey));
        systemOnBattery = !CFEqual(value, CFSTR(kIOPSACPowerValue));
        if (systemOnBattery) break;
      }

      CFRelease(list);
    }

    CFRelease(info);
  }

#else
  try {
    const char *sysBase = "/sys/class/power_supply";
    const char *procBase = "/proc/acpi/ac_adapter";

    bool useSys = SystemUtilities::exists(sysBase);
    bool useProc = !useSys && SystemUtilities::exists(procBase);

    if (!useSys && !useProc) return;

    string base = useSys ? sysBase : procBase;

    // Has battery
    const char *batNames[] = {"/BAT", "/BAT0", "/BAT1", 0};
    for (int i = 0; !systemHasBattery && batNames[i]; i++)
      systemHasBattery = SystemUtilities::exists(base + batNames[i]);

    // On battery
    if (systemHasBattery) {
      if (useSys) {
        const char *acNames[] = { "/AC0/online", "/AC/online", 0};

        for (int i = 0; acNames[i]; i++) {
          string path = base + acNames[i];

          if (SystemUtilities::exists(path))
            systemOnBattery = String::trim(SystemUtilities::read(path)) == "0";
        }

      } else if (useProc) {
        string path = base + "/AC0/state";

        if (SystemUtilities::exists(path)) {
          vector<string> tokens;
          String::tokenize(SystemUtilities::read(path), tokens);

          if (tokens.size() == 2 && tokens[0] == "state")
            systemOnBattery = tokens[1] != "on-line";
        }
      }
    }
  } CBANG_CATCH_ERROR;
#endif
}
