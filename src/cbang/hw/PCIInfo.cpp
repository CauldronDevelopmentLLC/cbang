/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include "PCIInfo.h"

#include <cbang/String.h>
#include <cbang/Catch.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/log/Logger.h>


#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <cstring>

#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#endif


using namespace std;
using namespace cb;


PCIInfo::PCIInfo() {detect();}


void PCIInfo::add(uint16_t vendorID, uint16_t deviceID, uint8_t busID,
                  uint8_t slotID, uint8_t functionID, const string &name) {
  devices.push_back(
    PCIDevice(vendorID, deviceID, busID, slotID, functionID, name));
}


#if defined(_WIN32)
static string getDevRegProp(HDEVINFO &info, SP_DEVINFO_DATA &dev, DWORD prop) {
  // Get size
  DWORD size = 0;
  SetupDiGetDeviceRegistryProperty(info, &dev, prop, 0, 0, 0, &size);
  if (!size) return "";

  // Get property
  SmartPointer<char>::Array buf = new char[size];
  if (!SetupDiGetDeviceRegistryProperty(info, &dev, prop, 0, (PBYTE)buf.get(),
                                        size, 0)) return "";

  return string(buf.get(), strlen(buf.get()));
}
#endif


#if defined(__APPLE__)
static CFDataRef getIORegistryProperty(io_service_t dev, CFStringRef name) {
  CFDataRef ref = (CFDataRef)IORegistryEntryCreateCFProperty(
    dev, name, kCFAllocatorDefault, 0);

  if (ref && CFDataGetTypeID() != CFGetTypeID(ref)) {
    CFRelease(ref);
    return 0;
  }

  return ref;
}

static uint32_t getIORegistryNumber(io_service_t dev, CFStringRef name) {
  CFDataRef ref = getIORegistryProperty(dev, name);
  if (!ref) return 0;

  uint32_t value = 0;
  switch (CFDataGetLength(ref)) {
  case 0: break;
  case 1: value = *(uint8_t *)CFDataGetBytePtr(ref); break;
  case 2:
  case 3: value = *(uint16_t *)CFDataGetBytePtr(ref); break;
  default: value = *(uint32_t *)CFDataGetBytePtr(ref); break;
  }

  CFRelease(ref);

  return value;
}


static string getIORegistryString(io_service_t dev, CFStringRef name) {
  CFDataRef ref = getIORegistryProperty(dev, name);
  if (!ref) return string();

  string value((const char *)CFDataGetBytePtr(ref), CFDataGetLength(ref));

  CFRelease(ref);

  return value;
}
#endif


void PCIInfo::detect() {
  devices.clear();

  try {
#if defined(_WIN32)
    HDEVINFO info =
      SetupDiGetClassDevs(0, "PCI", 0, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (info == INVALID_HANDLE_VALUE) return;

    SP_DEVINFO_DATA dev;
    dev.cbSize = sizeof(SP_DEVINFO_DATA);
    for (int i = 0; SetupDiEnumDeviceInfo(info, i, &dev); i++) {
      string prop = getDevRegProp(info, dev, SPDRP_HARDWAREID);

      if (prop.substr(0, 8) != "PCI\\VEN_" || prop.size() < 21) continue;

      uint16_t vendorID = String::parseU16("0x" + prop.substr(8, 4));
      uint16_t deviceID = String::parseU16("0x" + prop.substr(17, 4));

      uint8_t busID = 0;
      uint8_t slotID = 0;
      uint8_t functionID = 0;

      DWORD bus = 0;
      if (SetupDiGetDeviceRegistryProperty(info, &dev, SPDRP_BUSNUMBER, 0,
                                           (PBYTE)&bus, sizeof(bus), 0))
        busID = (uint8_t)bus;

      DWORD addr = 0;
      if (SetupDiGetDeviceRegistryProperty(info, &dev, SPDRP_ADDRESS, 0,
                                           (PBYTE)&addr, sizeof(addr), 0)) {
        slotID = (addr >> 16) & 0xff;
        functionID = addr & 0xff;
      }

      string description = getDevRegProp(info, dev, SPDRP_DEVICEDESC);

      add(vendorID, deviceID, busID, slotID, functionID, description);
    }

#elif defined(__APPLE__)
    // Detect PCI devices on OSX
    io_iterator_t iter;
    kern_return_t kr;
    if (IOServiceGetMatchingServices(
          kIOMasterPortDefault, IOServiceMatching("IOPCIDevice"), &iter) ==
        kIOReturnSuccess) {

      // Iterate through PCI device results
      io_service_t dev;
      while ((dev = IOIteratorNext(iter))) {
        uint16_t vendorID = getIORegistryNumber(dev, CFSTR("vendor-id"));
        uint16_t deviceID = getIORegistryNumber(dev, CFSTR("device-id"));

        if (!vendorID || !deviceID) continue;

        uint8_t busID = 0;
        uint8_t slotID = 0;
        uint8_t functionID = 0;

        CFDataRef reg = getIORegistryProperty(dev, CFSTR("reg"));
        if (reg) {
          if (3 < CFDataGetLength(reg)) {
            uint8_t *data = (uint8_t *)CFDataGetBytePtr(reg);

            busID = data[2];
            slotID = data[1] >> 3;
            functionID = data[1] & 7;
          }

          CFRelease(reg);
        }

        string description = getIORegistryString(dev, CFSTR("model"));
        if (description.empty()) {
          io_name_t devName;
          kr = IORegistryEntryGetName(dev, devName);
          if (kr == KERN_SUCCESS) description = devName;
        }

        // Add it
        add(vendorID, deviceID, busID, slotID, functionID, description);
      }

      IOObjectRelease(iter);
    }

#else
    SmartPointer<iostream> f =
      SystemUtilities::open("/proc/bus/pci/devices", ios::in);

    while (!f->fail()) {
      // Parse Bus, Slot, Function
      char bus[2];
      char devFun[2];
      f->read(bus, 2);
      f->read(devFun, 2);
      if (f->fail()) break;

      uint8_t busID      = String::parseU8("0x" + string(bus, 2));
      uint8_t devFunID   = String::parseU8("0x" + string(devFun, 2));
      uint8_t slotID     = devFunID >> 3; // Upper 5 bits
      uint8_t functionID = devFunID & 7;  // Lower 3 bits

      // Find first tab
      while (f->get() != '\t' && !f->fail()) continue;

      // Parse vendor and device IDs
      char vendor[4];
      char device[4];
      f->read(vendor, 4);
      f->read(device, 4);
      if (f->fail()) break;

      uint16_t vendorID = String::parseU16("0x" + string(vendor, 4));
      uint16_t deviceID = String::parseU16("0x" + string(device, 4));

      // TODO Need device description
      add(vendorID, deviceID, busID, slotID, functionID);

      // Skip to end of line
      while (f->get() != '\n' && !f->fail()) continue;
    }

#endif
  } CBANG_CATCH_WARNING;

  // TODO FreeBSD PCI enumeration
}
