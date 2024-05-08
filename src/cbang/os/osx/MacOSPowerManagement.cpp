/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
                Copyright (c) 2003-2021, Cauldron Development LLC
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

#include "MacOSPowerManagement.h"
#include "MacOSString.h"

#include <cbang/os/SystemUtilities.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

using namespace cb;
using namespace std;


MacOSPowerManagement::~MacOSPowerManagement() {_setAllowSleep(true);}


void MacOSPowerManagement::_setAllowSleep(bool allow) {
  if (!allow) {
    MacOSString name(
      SystemUtilities::basename(SystemUtilities::getExecutablePath()));

    if (IOPMAssertionCreateWithName(
          kIOPMAssertionTypeNoIdleSleep, kIOPMAssertionLevelOn, name,
          &systemAssertionID) != kIOReturnSuccess)
      systemAssertionID = 0;

  } else if (systemAssertionID) {
    IOPMAssertionRelease(systemAssertionID);
    systemAssertionID = 0;
  }
}


unsigned MacOSPowerManagement::_getIdleSeconds() {
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


bool MacOSPowerManagement::_getHasBattery() {
  MacOSRef<CFTypeRef> info(IOPSCopyPowerSourcesInfo());
  if (!info) return false;

  MacOSRef<CFArrayRef> list(IOPSCopyPowerSourcesList(info));

  return list && CFArrayGetCount(list);
}


bool MacOSPowerManagement::_getOnBattery() {
  MacOSRef<CFTypeRef> info(IOPSCopyPowerSourcesInfo());
  if (!info) return false;

  MacOSRef<CFArrayRef> list(IOPSCopyPowerSourcesList(info));

  if (list)
    for (CFIndex i = 0; i < CFArrayGetCount(list); i++) {
      CFTypeRef item          = CFArrayGetValueAtIndex(list, i);
      CFDictionaryRef battery = IOPSGetPowerSourceDescription(info, item);
      CFTypeRef value =
        CFDictionaryGetValue(battery, CFSTR(kIOPSPowerSourceStateKey));

      if (!CFEqual(value, CFSTR(kIOPSACPowerValue))) return true;
    }

  return false;
}
