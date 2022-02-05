/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2022, Cauldron Development LLC
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


#include <cbang/Exception.h>
#include <cbang/os/CPUID.h>
#include <cbang/enum/CPUFeature.h>
#include <cbang/util/SaveOStreamConfig.h>
#include <cbang/util/Hex.h>

#include <iostream>
#include <iomanip>


using namespace std;
using namespace cb;


#if defined(_WIN32) || defined(__i386__) || defined(__x86_64)
#define CBANG_HWPLATFORM_INTEL 1
#elif defined(__aarch64__)
#define CBANG_HWPLATFORM_AARCH64 1
#endif


#if defined(CBANG_HWPLATFORM_INTEL)
void PrintCPUIDFunction(unsigned function) {
  SaveOStreamConfig save(cout);

  CPUID cpuid;
  cpuid.cpuID(function);

  cout
    << "CPUID." << Hex(function,    8)
    << " EAX="  << Hex(cpuid.EAX(), 8)
    << " EBX="  << Hex(cpuid.EBX(), 8)
    << " ECX="  << Hex(cpuid.ECX(), 8)
    << " EDX="  << Hex(cpuid.EDX(), 8)
    << endl;
}
#endif // CBANG_HWPLATFORM_INTEL


int main(int argc, char *argv[]) {
  CPUID cpuid;

  uint32_t logical;
  uint32_t cores;
  uint32_t threads;
  cpuid.getCPUCounts(logical, cores, threads);

  cout
    << setw(13) << "Vendor: "   << cpuid.getCPUVendor()   << '\n'
    << setw(13) << "Brand: "    << cpuid.getCPUBrand()    << '\n'
    << setw(13) << "Family: "   << cpuid.getCPUFamily()   << '\n'
    << setw(13) << "Model: "    << cpuid.getCPUModel()    << '\n'
    << setw(13) << "Stepping: " << cpuid.getCPUStepping() << '\n'
    << setw(13) << "Cores: "    << cores                  << '\n'
    << setw(13) << "Threads: "  << threads                << '\n'
    << setw(13) << "Logical: "  << logical                << '\n';

  cout << setw(12) << "Features: ";
  cpuid.printCPUFeatures(cout);
  cout << "\n\n";

#if defined(CBANG_HWPLATFORM_INTEL)
  for (unsigned i = 1; i < 14; i++) PrintCPUIDFunction(i);
  for (unsigned i = 0x80000000; i <= 0x80000008; i++) PrintCPUIDFunction(i);

  cout
    << '\n'
    << "CPUID.0x00000001.EAX[23:16] = " << cpuid.cpuID(1).EBX(23, 16) << '\n'
    << "CPUID.0x00000004.EAX[25:14] = " << cpuid.cpuID(4).EAX(25, 14) << '\n'
    << "CPUID.0x00000004.EAX[31:26] = " << cpuid.cpuID(4).EAX(31, 26) << '\n'
    << "CPUID.0x80000008.ECX[ 7: 0] = " << cpuid.cpuID(0x80000008).EAX(7, 0)
    << '\n';

#elif defined(CBANG_HWPLATFORM_AARCH64)
  cout
    << "CPUID.MIDR_EL1         = " << Hex(cpuid.MIDR_EL1(), 8) << '\n'
    << "CPUID.ID_AA64ISAR0_EL1 = " << Hex(cpuid.ID_AA64ISAR0_EL1(), 16) << '\n'
    << "CPUID.ID_AA64ISAR1_EL1 = " << Hex(cpuid.ID_AA64ISAR1_EL1(), 16) << '\n'
    << "CPUID.ID_AA64PFR0_EL1  = " << Hex(cpuid.ID_AA64PFR0_EL1(),  16) << '\n'
    << '\n';
#endif // CBANG_HWPLATFORM_AARCH64

  return 0;
}
