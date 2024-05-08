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

#pragma once

#include "PCIDevice.h"

#include <cbang/util/Singleton.h>


namespace cb {
  class PCIInfo : public Singleton<PCIInfo> {
    typedef std::vector<PCIDevice> devices_t;
    devices_t devices;

  public:
    PCIInfo(Inaccessible);

    typedef devices_t::const_iterator iterator;
    iterator begin() const {return devices.begin();}
    iterator end() const {return devices.end();}

  protected:
    void add(uint16_t vendorID, uint16_t deviceID, uint8_t busID,
             uint8_t slotID, uint8_t functionID,
             const std::string &name = std::string());

    void detect();
  };
}
