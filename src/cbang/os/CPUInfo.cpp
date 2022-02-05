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

#include "CPUInfo.h"
#include "CPUInfoX86.h"
#include "CPUInfoAArch64.h"

#include <cbang/String.h>

#include <iomanip>

using namespace cb;
using namespace std;


SmartPointer<CPUInfo> CPUInfo::create() {
#if defined(_WIN32) || defined(__i386__) || defined(__x86_64)
  return new CPUInfoX86;

#elif defined(__aarch64__)
  return new CPUInfoAArch64;

#else
  return new CPUInfo;
#endif
}


bool CPUInfo::hasFeature(const string &feature) const {
  return features.find(String::toUpper(feature)) != features.end();
}


void CPUInfo::dump(ostream &stream) const {
  const unsigned w = 12;

  stream
    << setw(w) << "Vendor: "   << getVendor()                 << '\n'
    << setw(w) << "Brand: "    << getBrand()                  << '\n'
    << setw(w) << "Family: "   << getFamily()                 << '\n'
    << setw(w) << "Model: "    << getModel()                  << '\n'
    << setw(w) << "Stepping: " << getStepping()               << '\n'
    << setw(w) << "Cores: "    << getPhysicalCPUCount()       << '\n'
    << setw(w) << "Threads: "  << getThreadsPerCore()         << '\n'
    << setw(w) << "Logical: "  << getLogicalCPUCount()        << '\n'
    << setw(w) << "Features: " << String::join(getFeatures()) << '\n'
    << "\nRegisters:\n";

  dumpRegisters(stream, 2);
}
