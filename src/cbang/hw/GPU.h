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

#pragma once

#include "GPUType.h"

#include <cbang/json/Serializable.h>

namespace cb {
  namespace JSON {
    class Sink;
    class Value;
  }
}


namespace cb {
  class GPU : public JSON::Serializable {
    uint16_t vendorID;
    uint16_t deviceID;
    uint16_t type;
    uint16_t species;
    std::string description;

  public:
    GPU(uint16_t vendorID = 0, uint16_t deviceID = 0, uint16_t type = 0,
        uint16_t species = 0, const std::string &description = "") :
      vendorID(vendorID), deviceID(deviceID), type(type), species(species),
      description(description) {}
    GPU(const JSON::Value &value) {read(value);}

    uint16_t getVendorID() const {return vendorID;}
    uint16_t getDeviceID() const {return deviceID;}
    GPUType getType() const {return (GPUType::enum_t)type;}
    uint16_t getSpecies() const {return species;}
    const std::string &getDescription() const {return description;}

    void setVendorID(uint16_t x) {vendorID = x;}
    void setDeviceID(uint16_t x) {deviceID = x;}
    void setType(uint16_t x) {type = x;}
    void setSpecies(uint16_t x) {species = x;}
    void setDescription(const std::string &x) {description = x;}

    bool operator<(const GPU &gpu) const;

    // From JSON::Serializable
    void read(const JSON::Value &value) override;
    void write(JSON::Sink &sink) const override;
  };
}
