/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2022, Cauldron Development LLC
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

#include "CPURegsX86.h"

#include <cstring>

#if 1500 < _MSC_VER
#include <intrin.h>
#endif

using namespace std;
using namespace cb;


CPURegsX86::CPURegsX86() {memset(regs, 0, sizeof(regs));}


CPURegsX86 &CPURegsX86::cpuID(uint32_t _eax, uint32_t _ebx, uint32_t _ecx,
                              uint32_t _edx) {
  for (int i = 0; i < 4; i++) regs[i] = 0;

#ifdef _WIN32
#if 1500 < _MSC_VER && (defined(_M_IX86) || defined(_M_AMD64))
  // Note that 64-bit MSVC compiler does not support __asm()
  int cpuInfo[4];
  __cpuidex(cpuInfo, (int)_eax, (int)_ecx);
  for (int i = 0; i < 4; i++) regs[i] = (uint32_t)cpuInfo[i];
#endif

#else // _WIN32

#if defined(__i386__) && defined(__PIC__)
  asm volatile
    ("xchgl\t%%ebx, %1\n\t"
     "cpuid\n\t"
     "xchgl\t%%ebx, %1\n\t"
     : "=a" (regs[0]), "=r" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
     : "a" (_eax), "r" (_ebx), "c" (_ecx), "d" (_edx));

#elif defined(__x86_64) || defined(__i386__)
  asm volatile
    ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
     : "a" (_eax), "b" (_ebx), "c" (_ecx), "d" (_edx));
#endif

#endif // _WIN32

  return *this;
}


uint32_t CPURegsX86::getBits(uint32_t x, unsigned start, unsigned end) {
  return (x >> end) & (~((uint32_t)0) >> (31 - (start - end)));
}


string CPURegsX86::getCPUBrand() {
  cpuID(0x80000000); // Get max extension function

  if (EAX() & 0x80000000 && EAX() >= 0x80000004) {
    string brand;

    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
      cpuID(i);
      brand += string((const char *)getRegs(), 16);
    }

    return brand.c_str(); // Strips trailing zeros
  }

  return "Unknown";
}


string CPURegsX86::getCPUVendor() {
  uint32_t vendor[3];

  cpuID(0);

  vendor[0] = EBX();
  vendor[1] = EDX();
  vendor[2] = ECX();

  return string((char *)vendor, 12);
}


uint32_t CPURegsX86::getCPUSignature() {
  cpuID(1);
  return EAX();
}


uint64_t CPURegsX86::getCPUFeatures() {
  cpuID(1);
  return (uint64_t)ECX() << 32 | EDX();
}


uint64_t CPURegsX86::getCPUExtendedFeatures() {
  cpuID(7);
  return (uint64_t)ECX() << 32 | EBX();
}


uint64_t CPURegsX86::getCPUFeatures80000001() {
  cpuID(0x80000001);
  return (uint64_t)ECX() << 32 | EDX();
}


bool CPURegsX86::cpuHasFeature(CPUFeature feature) {
  return getCPUFeatures() & (1UL << (CPUFeature::enum_t)feature);
}


bool CPURegsX86::cpuHasExtendedFeature(CPUExtendedFeature feature) {
  return getCPUExtendedFeatures() &
    (1UL << (CPUExtendedFeature::enum_t)feature);
}


bool CPURegsX86::cpuHasFeature80000001(CPUFeature80000001 feature) {
  return getCPUFeatures80000001() &
    (1UL << (CPUFeature80000001::enum_t)feature);
}


unsigned CPURegsX86::getCPUFamily() {
  uint32_t signature = getCPUSignature();
  uint32_t family = getBits(signature, 11, 8);

  if (getCPUVendor() != "AuthenticAMD" || family == 15)
    family += getBits(signature, 27, 20);

  return family;
}


unsigned CPURegsX86::getCPUModel() {
  uint32_t signature = getCPUSignature();
  uint32_t model = getBits(signature, 7, 4);

  if (getCPUVendor() != "AuthenticAMD" || 15 <= getCPUFamily())
    model += getBits(signature, 19, 16) << 4;

  return model;
}


unsigned CPURegsX86::getCPUStepping() {return getBits(getCPUSignature(), 3, 0);}


void CPURegsX86::getCPUFeatureNames(set<string> &names) {
  uint64_t features = getCPUFeatures();

  for (unsigned i = 0; i < CPUFeature::getCount(); i++)
    if (features & (1 << i))
      names.insert(CPUFeature(CPUFeature::getValue(i)).toString());
}
