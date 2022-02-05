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

#include "CPUInfoX86.h"

using namespace cb;
using namespace std;


CPUInfoX86::CPUInfoX86() {
  vendor    = getCPUVendor();
  brand     = getCPUBrand();
  signature = getCPUSignature();
  family    = getCPUFamily();
  model     = getCPUModel();
  stepping  = getCPUStepping();
  physical  = 1;
  threads   = 1;

  // Physical cores and threads
  cpuID(1);
  physical = EBX(23, 16);

  if (vendor == "GenuineIntel") {
    cpuID(4); // Get DCP cache info
    physical = EAX(31, 26) + 1; // APIC IDs reserved for the package

    if (cpuHasFeature(CPUFeature::FEATURE_HTT)) {
      uint32_t threadsPerCache = EAX(25, 14) + 1;

      if (1 < threadsPerCache) {
        threads = threadsPerCache;
        physical /= threadsPerCache;
      }
    }

  } else if (vendor == "AuthenticAMD" || vendor == "HygonGenuine") {
    cpuID(0x80000008);
    physical = ECX(7, 0) + 1;
  }

  if (!physical) physical = 1;

  // Features
  getCPUFeatureNames(features);
}


void CPUInfoX86::dumpRegisters(ostream &stream, unsigned indent) const {
  CPURegsX86().dump(stream, indent);
}
