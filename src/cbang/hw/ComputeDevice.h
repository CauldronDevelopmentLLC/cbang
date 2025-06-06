/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include <cbang/util/Version.h>

#include <ostream>
#include <cstdint>


namespace cb {
  struct ComputeDevice {
    std::string name;
    std::string platform;
    VersionU16  driverVersion;
    VersionU16  computeVersion;
    int32_t     vendorID       = -1;
    int32_t     platformIndex  = -1;
    int32_t     deviceIndex    = -1;
    bool        gpu            = false;
    int         pciBus         = -1;
    int         pciSlot        = -1;
    int         pciFunction    = -1;
    std::string uuid;

    bool isValid() const;
    void print(std::ostream &stream) const;
    bool isPCIValid() const;
    std::string getPCIID() const;
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const ComputeDevice &dev) {
    dev.print(stream);
    return stream;
  }
}
