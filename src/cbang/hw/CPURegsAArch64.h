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

#include <ostream>
#include <cstdint>


namespace cb {
  class CPURegsAArch64 {
    uint64_t regs[4];

  public:
    CPURegsAArch64();

    uint32_t MIDR_EL1(unsigned start = 31, unsigned end = 0) const
      {return getBits(regs[0], start, end);}
    uint64_t ID_AA64ISAR0_EL1(unsigned start = 63, unsigned end = 0) const
      {return getBits(regs[1], start, end);}
    uint64_t ID_AA64ISAR1_EL1(unsigned start = 63, unsigned end = 0) const
      {return getBits(regs[2], start, end);}
    uint64_t ID_AA64PFR0_EL1(unsigned start = 63, unsigned end = 0) const
      {return getBits(regs[3], start, end);}

    static uint64_t getBits(uint64_t x, unsigned start = 63, unsigned end = 0);

    std::string getCPUVendor() const;
    std::string getCPUBrand() const;
    uint32_t getCPUSignature() const;
    unsigned getCPUFamily() const;
    unsigned getCPUModel() const;
    unsigned getCPUStepping() const;
  };
}
