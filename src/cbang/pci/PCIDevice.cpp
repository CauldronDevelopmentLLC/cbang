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

#include "PCIDevice.h"

#include <cbang/String.h>
#include <cbang/json/Value.h>


using namespace std;
using namespace cb;


PCIDevice::PCIDevice(uint16_t vendorID, uint16_t deviceID, int16_t busID ,
                     int16_t slotID, int16_t functionID,
                     const string &description) :
  vendor(0), device(deviceID), bus(busID), slot(slotID), function(functionID),
  description(description) {
  setVendorID(vendorID);
}


void PCIDevice::setVendorID(uint16_t vendorID) {
  if (vendorID) vendor = PCIVendor::find(vendorID);
  else vendor = 0;
}


string PCIDevice::getID() const {
  string b = bus      == -1 ? "??" : String::printf("%02d", bus);
  string s = slot     == -1 ? "??" : String::printf("%02d", slot);
  string f = function == -1 ? "??" : String::printf("%02d", function);
  return b + ":" + s + ":" + f;
}


string PCIDevice::getVendorIDStr() const {
  return String::printf("0x%04x", getVendorID());
}


string PCIDevice::getDeviceIDStr() const {
  return String::printf("0x%04x", getDeviceID());
}


bool PCIDevice::operator<(const PCIDevice &d) const {
  if (bus           != d.bus)           return bus           < d.bus;
  if (slot          != d.slot)          return slot          < d.slot;
  if (function      != d.function)      return function      < d.function;
  if (getVendorID() != d.getVendorID()) return getVendorID() < d.getVendorID();
  return device < d.device;
}


void PCIDevice::read(const JSON::Value &value) {
  setVendorID(value.getU16("vendor"));
  device      = value.getU16("device");
  bus         = value.getS16("bus", -1);
  slot        = value.getS16("slot", -1);
  function    = value.getS16("function", -1);
  description = value.getString("description", "");
}


void PCIDevice::write(JSON::Sink &sink) const {
  sink.beginDict();

  sink.insert("vendor", getVendorID());
  sink.insert("device", device);
  if (0 <= bus)             sink.insert("bus",         bus);
  if (0 <= slot)            sink.insert("slot",        slot);
  if (0 <= function)        sink.insert("function",    function);
  if (!description.empty()) sink.insert("description", description);

  sink.endDict();
}
