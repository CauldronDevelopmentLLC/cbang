/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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
#include <cbang/util/Hex.h>

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


void CPUInfo::print(ostream &stream) const {
  const unsigned w = 20;

  stream
    << setw(w) << "Vendor: "    << getVendor()                 << '\n'
    << setw(w) << "Brand: "     << getBrand()                  << '\n'
    << setw(w) << "Family: "    << getFamily()                 << '\n'
    << setw(w) << "Model: "     << getModel()                  << '\n'
    << setw(w) << "Stepping: "  << getStepping()               << '\n'
    << setw(w) << "Physical: "  << getPhysicalCPUCount()       << '\n'
    << setw(w) << "Threading: " << getThreadsPerCore()         << '\n'
    << setw(w) << "Logical: "   << getLogicalCPUCount()        << '\n'
    << setw(w) << "Features: "  << String::join(getFeatures()) << '\n';

  for (auto it = registers.begin(); it != registers.end(); it++)
    stream << setw(w) << (it->first + ": ") << Hex(it->second, 16) << '\n';
}


void CPUInfo::write(JSON::Sink &sink) const {
  sink.beginDict();

  sink.insert("vendor",    getVendor());
  sink.insert("brand",     getBrand());
  sink.insert("family",    getFamily());
  sink.insert("model",     getModel());
  sink.insert("stepping",  getStepping());
  sink.insert("physical",  getPhysicalCPUCount());
  sink.insert("threading", getThreadsPerCore());
  sink.insert("logical",   getLogicalCPUCount());

  // Features
  sink.insertList("features");
  auto &features = getFeatures();
  for (auto it = features.begin(); it != features.end(); it++)
    sink.append(*it);
  sink.endList();

  // Registers
  sink.insertDict("registers");
  for (auto it = registers.begin(); it != registers.end(); it++)
    sink.insert(it->first, it->second);
  sink.endDict();

  sink.endDict();
}
