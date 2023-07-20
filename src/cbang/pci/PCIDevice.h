/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#pragma once

#include "PCIVendor.h"

#include <cbang/json/Serializable.h>

#include <string>
#include <set>
#include <ostream>
#include <cstdint>


namespace cb {
  class PCIDevice : public JSON::Serializable {
    const PCIVendor *vendor;
    uint16_t         device;
    int16_t          bus;
    int16_t          slot;
    int16_t          function;
    std::string      description;

  public:
    PCIDevice(uint16_t vendorID = 0, uint16_t deviceID = 0, int16_t busID = -1,
              int16_t slotID = -1, int16_t functionID = -1,
              const std::string &description = "");
    PCIDevice(const JSON::Value &value) {read(value);}

    const PCIVendor *getVendor() const {return vendor;}

    uint16_t    getVendorID  () const {return vendor ? vendor->getID()   : 0;}
    std::string getVendorName() const {return vendor ? vendor->getName() : "";}
    uint16_t    getDeviceID  () const {return device;}
    int16_t     getBusID     () const {return bus;}
    int16_t     getSlotID    () const {return slot;}
    int16_t     getFunctionID() const {return function;}
    const std::string &getDescription() const {return description;}

    void setVendorID   (uint16_t vendorID);
    void setDeviceID   (uint16_t device)      {this->device   = device;}
    void setBusID      (uint8_t  bus)         {this->bus      = bus;}
    void setSlotID     (uint8_t  slot)        {this->slot     = slot;}
    void setFunctionID (uint8_t  function)    {this->function = function;}
    void setDescription(const std::string &x) {description    = x;}

    std::string getID()          const;
    std::string getVendorIDStr() const;
    std::string getDeviceIDStr() const;

    bool operator<(const PCIDevice &d) const;

    // From JSON::Serializable
    void read(const JSON::Value &value);
    void write(JSON::Sink &sink) const;
  };
}
