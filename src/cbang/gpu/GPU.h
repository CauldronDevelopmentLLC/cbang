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

#include "GPUType.h"

#include <cbang/StdTypes.h>
#include <cbang/json/Serializable.h>
#include <cbang/pci/PCIDevice.h>


namespace cb {
  namespace JSON {
    class Sink;
    class Value;
  }
}


namespace cb {
  class GPU : public PCIDevice, public cb::JSON::Serializable {
    uint16_t type;
    uint16_t species;

  public:
    GPU(uint16_t vendorID = 0, uint16_t deviceID = 0,
        const std::string &name = "", uint16_t type = 0, uint16_t species = 0) :
      PCIDevice(vendorID, deviceID, -1, -1, -1, name),
      type(type), species(species) {}

    std::string getIndexKey() const;

    void setType(GPUType type) {this->type = type;}
    GPUType getType() const {return (GPUType::enum_t)type;}
    void setSpecies(uint16_t species) {this->species = species;}
    uint16_t getSpecies() const {return species;}

    std::string toString() const;
    std::string toRowString() const;

    std::string toJSONString() const;
    void parseJSON(const std::string &s);

    // From cb::JSON::Serializable
    void read(const cb::JSON::Value &value);
    void write(cb::JSON::Sink &sink) const;
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const GPU &gpu) {
    return stream << gpu.toString();
  }
}
