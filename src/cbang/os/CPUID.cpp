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

#include "CPUID.h"
#include "SystemUtilities.h"
#include "SysError.h"

#include <cbang/String.h>
#include <cbang/Catch.h>

#if 1500 < _MSC_VER
#include <intrin.h>
#endif

#if defined(__aarch64__) && defined(__linux__)
#include <string.h>
#include <unistd.h>
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif

#if defined(__aarch64__) && defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/machine.h>
#endif

#include <string.h>

using namespace std;
using namespace cb;


#if defined(_WIN32) || defined(__i386__) || defined(__x86_64)
#define CBANG_HWPLATFORM_INTEL 1

#elif defined(__aarch64__)
#define CBANG_HWPLATFORM_AARCH64 1
#endif


#if defined(__aarch64__) && defined(__APPLE__)
static uint32_t cpu_part() {
  uint32_t family;
  uint32_t subfamily;

  size_t len = sizeof(family);
  if (::sysctlbyname("hw.cpufamily", &family, &len, 0, 0) < 0)
    THROW("Can't read 'hw.cpufamily': " << SysError());

  len = sizeof(subfamily);
  if (::sysctlbyname("hw.cpusubfamily", &subfamily, &len, 0, 0) < 0)
    THROW("Can't read 'hw.cpusubfamily': " << SysError());

  // The numbers are taken from
  // https://github.com/apple/darwin-xnu/blob/master/osfmk/arm/cpuid.h
  // and /Library/Developer/CommandLineTools/SDKs/MacOSX11.1.sdk/System/
  //   Library/Frameworks/Kernel.framework/Versions/A/Headers/arm/cpuid.h
  bool arm_hg = subfamily == CPUSUBFAMILY_ARM_HG;
  switch (family) {
  case CPUFAMILY_ARM_CYCLONE:            return 1;                // A7
  case CPUFAMILY_ARM_TYPHOON:            return arm_hg ? 3 : 2;   // A8X  : A8
  case CPUFAMILY_ARM_TWISTER:            return arm_hg ? 5 : 4;   // A9X  : a9
  case CPUFAMILY_ARM_HURRICANE:          return arm_hg ? 7 : 6;   // A10X : A10
  case CPUFAMILY_ARM_MONSOON_MISTRAL:    return 9;                // A11
  case CPUFAMILY_ARM_VORTEX_TEMPEST:     return arm_hg ? 17 : 12; // A12X : A12
  case CPUFAMILY_ARM_LIGHTNING_THUNDER:  return 19;               // A13
  case CPUFAMILY_ARM_FIRESTORM_ICESTORM: return arm_hg ? 35 : 33; // M1   : A14
  default: return 0;
  }
}
#endif // defined(__aarch64__) && defined(__APPLE__)


CPUID::CPUID() {
#if defined(CBANG_HWPLATFORM_AARCH64)
  memset(regs, 0, sizeof(regs));

#if defined(__linux__)
  if ((getauxval(AT_HWCAP) & HWCAP_CPUID)) {
    // Use AArch64 feature registers.  See
    //   https://www.kernel.org/doc/html/latest/arm64/cpu-feature-registers.html

    asm("mrs %0, MIDR_EL1"         : "=r" (regs[0]));
    asm("mrs %0, ID_AA64ISAR0_EL1" : "=r" (regs[1]));
    asm("mrs %0, ID_AA64ISAR1_EL1" : "=r" (regs[2]));
    asm("mrs %0, ID_AA64PFR0_EL1"  : "=r" (regs[3]));
  }

  if (!regs[0]) {
    // Try to read and parse MIDR_EL1 register, exposed by the Linux kernel
    try {
      string filename =
        "/sys/devices/system/cpu/cpu0/regs/identification/midr_el1";

      if (SystemUtilities::exists(filename))
        regs[0] = String::parseU64(SystemUtilities::read(filename));

    } CATCH_WARNING;
  }

  if (!regs[1] || !regs[3]) {
    // Try to parse /proc/cpuinfo and fill the register appropriately
    string filename = "/proc/cpuinfo";
    if (SystemUtilities::exists(filename)) {
      auto f = SystemUtilities::iopen(filename);

      while (!f->fail()) {
        string line = SystemUtilities::getline(*f);
        vector<string> parts;
        String::tokenize(line, parts, ":", false, 2);

        if (parts.size() != 2) continue;
        string name  = String::trim(parts[0]);
        string value = String::trim(parts[1]);

        if (name == "CPU implementer")
          regs[0] |= (String::parseU64(value) & 0xff) << 24;

        if (name == "CPU part")
          regs[0] |= (String::parseU64(value) & 0xfff) << 4;

        if (name == "CPU variant")
          regs[0] |= (String::parseU64(value) & 0xf) << 20;

        if (name == "CPU revision")
          regs[0] |= String::parseU64(value) & 0xf;

        if (name == "CPU architecture") {
          uint64_t v = String::parseU64(value);
          if (v == 7 || v == 8) regs[0] |= 0xf << 16;
        }

        if (name == "Features") {
          vector<string> features;
          String::tokenize(value, features);

          for (unsigned i = 0; i < features.size(); i++) {
            if (features[i] == "fphp")     regs[3] |= 1ULL << 16;
            if (features[i] == "asimdhp")  regs[3] |= 1ULL << 20;
            if (features[i] == "aes")      regs[3] |= 1ULL << 4;
            if (features[i] == "pmull")    regs[3] |= 1ULL << 5;
            if (features[i] == "sha1")     regs[3] |= 1ULL << 8;
            if (features[i] == "sha2")     regs[3] |= 1ULL << 12;
            if (features[i] == "sha3")     regs[3] |= 1ULL << 32;
            if (features[i] == "crc32")    regs[3] |= 1ULL << 16;
            if (features[i] == "atomics")  regs[3] |= 1ULL << 21;
            if (features[i] == "asimdrdm") regs[3] |= 1ULL << 28;
            if (features[i] == "asimddp")  regs[3] |= 1ULL << 44;
          }
        }
      }
    }
  }

  // If we're unable to retrieve the MIDR_EL1, assume Arm Cortex-A53 rev 0
  if (!regs[0]) regs[0] = 0x410fd030;

  // If the "Architecture" part of the MIDR_EL1 isn't set, force it to 0xf
  if (!(regs[0] & 0xf0000)) regs[0] |= 0xf0000;

  // If the "EL0" part of the ID_AA64PFR0_EL1 isn't set, force AArch64-only mode
  if (!(regs[3] & 0xf)) regs[3] |= 1;

  // If the "EL1" part of the ID_AA64PFR0_EL1 isn't set, force AArch64-only mode
  if (!(regs[3] & 0xf0)) regs[3] |= 16;

#elif defined(__APPLE__)
  uint32_t type;
  size_t len = sizeof(type);
  if (::sysctlbyname("hw.cputype", &type, &len, 0, 0) < 0)
    THROW("Can't read 'hw.cputype': " << SysError());

  if (type != CPU_TYPE_ARM64)
    THROW("Expected hw.cputype=0x" << hex << CPU_TYPE_ARM64 << " was 0x"
          << type);

  // Set "implementer" field of the MIDR_EL1
  // 0x61 means Apple (CPU_VID_APPLE at
  // https://github.com/apple/darwin-xnu/blob/master/osfmk/arm/cpuid.h)
  regs[0] |= 0x61 << 24;

  // Set the "Architecture" field of the MIDR_EL1 to 0xf.
  // This is a requirement for ARMv8-A profile.
  // See https://developer.arm.com/docs/ddi0595/d/external-system-registers/
  //   midr_el1#Architecture_19 for details.
  regs[0] |= 0xf << 16;

  // Set "part" field of MIDR_EL1
  regs[0] |= (cpu_part() & 0xfff) << 4;

  // Set the "EL0" and "EL1" parts if the ID_AA64PFR0_EL1 to AArch64-only mode
  regs[3] |= 0x11;

  uint32_t val;
  len = sizeof(val);
  if (!::sysctlbyname("hw.optional.neon_fp16", &val, &len, 0, 0) && val == 1)
    regs[3] |= 1ULL << 16;

  len = sizeof(val);
  if (!::sysctlbyname("hw.optional.neon_hpfp", &val, &len, 0, 0) && val == 1)
    regs[3] |= 1ULL << 20;

  len = sizeof(val);
  if (!::sysctlbyname("hw.optional.armv8_crc32", &val, &len, 0, 0) && val == 1)
    regs[1] |= 1ULL << 16;

  len = sizeof(val);
  if (!::sysctlbyname("hw.optional.armv8_sha1", &val, &len, 0, 0) && val == 1)
    regs[1] |= 1ULL << 8;

  len = sizeof(val);
  if (!::sysctlbyname("hw.optional.armv8_sha2", &val, &len, 0, 0) && val == 1)
    regs[1] |= 1ULL << 12;

  len = sizeof(val);
  if (!::sysctlbyname("hw.optional.armv8_1_atomics", &val, &len, 0, 0) &&
      val == 1) regs[1] |= 1ULL << 21;

  len = sizeof(val);
  if (!::sysctlbyname("hw.optional.armv8_2_sha3", &val, &len, 0, 0) && val == 1)
    regs[1] |= 1ULL << 32;

  len = sizeof(val);
  if (!::sysctlbyname("hw.optional.armv8_2_sha512", &val, &len, 0, 0) &&
      val == 1) {
    // Arm Architecture Reference Manual, FEAT_SHA512 requires FEAT_SHA1
    regs[1] |= 1ULL << 8;
    // Clear SHA2 field.  Allowed values are 0b0000, 0b0001, and 0b0010.
    regs[1] &= ~(3ULL << 12);
    // Now, set SHA2 field to the value of 0b0010
    regs[1] |= 2ULL << 12;
  }

#else
#error "Cannot retrieve ARM CPU ID on this OS"
#endif

#endif // CBANG_HWPLATFORM_AARCH64
}


#if defined(CBANG_HWPLATFORM_INTEL)
CPUID &CPUID::cpuID(uint32_t _eax, uint32_t _ebx, uint32_t _ecx,
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
#endif // CBANG_HWPLATFORM_INTEL


#if defined(CBANG_HWPLATFORM_INTEL)
uint32_t CPUID::getBits(uint32_t x, unsigned start, unsigned end) {
  return (x >> end) & (~((uint32_t)0) >> (31 - (start - end)));
}


#elif defined(CBANG_HWPLATFORM_AARCH64)
uint64_t CPUID::getBits(uint64_t x, unsigned start, unsigned end) {
  return (x >> end) & (~((uint64_t)0) >> (63 - (start - end)));
}
#endif // CBANG_HWPLATFORM_AARCH64


string CPUID::getCPUBrand() {
#if defined(CBANG_HWPLATFORM_INTEL)
  cpuID(0x80000000); // Get max extension function

  if (EAX() & 0x80000000 && EAX() >= 0x80000004) {
    string brand;

    for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
      cpuID(i);
      brand += string((const char *)getRegs(), 16);
    }

    return brand.c_str(); // Strips trailing zeros

  } else return "Unknown";

#elif defined(CBANG_HWPLATFORM_AARCH64)
  switch (MIDR_EL1(31, 24)) {
    case 0x41: // Arm
      switch (MIDR_EL1(15, 4)) {
        case 0xd0c: return "Neoverse N";
        case 0xd4a: return "Neoverse E";
        default:    return "Cortex-A";
      }

    case 0x51: return "Snapdragon"; // Qualcomm
    case 0x53: return "Exynos";     // Samsung
    case 0x42: return "Cortex-A";   // Broadcom
    case 0x43:                      // Cavium
    case 0x56: return "ThunderX";   // Marvell

    case 0x4e: // NVIDIA
      switch (MIDR_EL1(15, 4)) {
        case 0x000:
        case 0x003: return "Denver";
        default:    return "Carmel";
      }

    case 0x61: // Apple
      switch (MIDR_EL1(15, 4)) {
        case 0x01: return "Apple A7";
        case 0x02: return "Apple A8";
        case 0x03: return "Apple A8X";
        case 0x04: return "Apple A9";
        case 0x05: return "Apple A9X";
        case 0x06: return "Apple A10";
        case 0x07: return "Apple A10X";
        case 0x08:
        case 0x09: return "Apple A11";
        case 0x0b:
        case 0x0c: return "Apple A12";
        case 0x10:
        case 0x11: return "Apple A12X";
        case 0x12:
        case 0x13: return "Apple A13";
        case 0x20:
        case 0x21: return "Apple A14";
        case 0x22:
        case 0x23: return "Apple M1";
        default:   return "Apple";
      }
    default: return "Arm Generic";
  }
#endif // CBANG_HWPLATFORM_AARCH64
}


string CPUID::getCPUVendor() {
#if defined(CBANG_HWPLATFORM_INTEL)
  uint32_t vendor[3];

  cpuID(0);

  vendor[0] = EBX();
  vendor[1] = EDX();
  vendor[2] = ECX();

  return string((char *)vendor, 12);

#elif defined(CBANG_HWPLATFORM_AARCH64)
  switch (MIDR_EL1(31, 24)) {
    case 0x41: return "Arm";
    case 0x42: return "Broadcom";
    case 0x43: return "Cavium";
    case 0x44: return "DEC";
    case 0x46: return "Fujitsu";
    case 0x48: return "HiSilicon";
    case 0x49: return "Infineon";
    case 0x4e: return "NVIDIA";
    case 0x50: return "APM";
    case 0x51: return "Qualcomm";
    case 0x53: return "Samsung";
    case 0x56: return "Marvell";
    case 0x66: return "Faraday";
    case 0x69: return "Intel";
    case 0xc0: return "Ampere";
    case 0x61: return "Apple";
    default:   return "UNKNOWN";
  }
#endif // CBANG_HWPLATFORM_AARCH64
}


uint32_t CPUID::getCPUSignature() {
#if defined(CBANG_HWPLATFORM_INTEL)
  cpuID(1);
  return EAX();

#elif defined(CBANG_HWPLATFORM_AARCH64)
  return ID_AA64ISAR0_EL1();
#endif // CBANG_HWPLATFORM_AARCH64
}


uint64_t CPUID::getCPUFeatures() {
#if defined(CBANG_HWPLATFORM_INTEL)
  cpuID(1);
  return (uint64_t)ECX() << 32 | EDX();

#elif defined(CBANG_HWPLATFORM_AARCH64)
  // We should return the ID_AA64ISAR0_EL1 here. However, there's a lot of code,
  // which treats the returned value as Intel-specific one. So, for the safety
  // reason, it's better to return 0 here, till the moment when all the places,
  // treating getCPUFeatures as an Intel-specific value, are fixed.
  // return ID_AA64ISAR0_EL1();
  return 0;

#endif // CBANG_HWPLATFORM_AARCH64
}


uint64_t CPUID::getCPUExtendedFeatures() {
#if defined(CBANG_HWPLATFORM_INTEL)
  cpuID(7);
  return (uint64_t)ECX() << 32 | EBX();

#elif defined(CBANG_HWPLATFORM_AARCH64)
  return 0;
#endif // CBANG_HWPLATFORM_INTEL
}


uint64_t CPUID::getCPUFeatures80000001() {
#if defined(CBANG_HWPLATFORM_INTEL)
  cpuID(0x80000001);
  return (uint64_t)ECX() << 32 | EDX();

#elif defined(CBANG_HWPLATFORM_AARCH64)
  return 0;
#endif // CBANG_HWPLATFORM_AARCH64
}


bool CPUID::cpuHasFeature(CPUFeature feature) {
  return getCPUFeatures() & (1UL << (CPUFeature::enum_t)feature);
}


bool CPUID::cpuHasExtendedFeature(CPUExtendedFeature feature) {
  return getCPUExtendedFeatures() &
    (1UL << (CPUExtendedFeature::enum_t)feature);
}


bool CPUID::cpuHasFeature80000001(CPUFeature80000001 feature) {
  return getCPUFeatures80000001() &
    (1UL << (CPUFeature80000001::enum_t)feature);
}


unsigned CPUID::getCPUFamily() {
#if defined(CBANG_HWPLATFORM_INTEL)
  uint32_t signature = getCPUSignature();
  uint32_t family = getBits(signature, 11, 8);

  if (getCPUVendor() != "AuthenticAMD" || family == 15)
    family += getBits(signature, 27, 20);

  return family;

#elif defined(CBANG_HWPLATFORM_AARCH64)
  return 8; // AArch64 is ARMv8
#endif // CBANG_HWPLATFORM_AARCH64
}


unsigned CPUID::getCPUModel() {
#if defined(CBANG_HWPLATFORM_INTEL)
  uint32_t signature = getCPUSignature();
  uint32_t model = getBits(signature, 7, 4);

  if (getCPUVendor() != "AuthenticAMD" || 15 <= getCPUFamily())
    model += getBits(signature, 19, 16) << 4;

  return model;
#elif defined(CBANG_HWPLATFORM_AARCH64)
  uint32_t implementer = MIDR_EL1(31, 24);
  uint32_t part = MIDR_EL1(15, 4);

  switch (implementer) {
    case 0x41: // Arm
      switch (part) {
        case 0xd03: return 53; // Cortex-A53
        case 0xd04: return 35; // Cortex-A35
        case 0xd05: return 55; // Cortex-A55
        case 0xd06: return 65; // Cortex-A65
        case 0xd07: return 57; // Cortex-A57
        case 0xd08: return 72; // Cortex-A72
        case 0xd09: return 73; // Cortex-A73
        case 0xd0a: return 75; // Cortex-A75
        case 0xd0b: return 76; // Cortex-A76
        case 0xd0d: return 77; // Cortex-A77
        case 0xd0c: return  1; // Neoverse N1
        case 0xd4a: return  1; // Neoverse E1
        default:    return  0;
      }

    case 0x51: // Qualcomm
      switch (part) {
        case 0x800: return 73; // Cortex-A73
        case 0x801: return 53; // Cortex-A53
        case 0x802: return 75; // Cortex-A75
        case 0x803: return 55; // Cortex-A55
        case 0x804: return 76; // Cortex-A76
        case 0x805: return 55; // Cortex-A55
        default:    return  0;
      }

    case 0x53: // Samsung
      switch (part) {
        case 0x001: return  1; // Exynos-M1
        case 0x002: return  2; // Exynos-M2
        case 0x003: return  3; // Exynos-M3
        case 0x004: return  4; // Exynos-M4
        default:    return  0;
      }

    default: return 0;
  }
#endif // CBANG_HWPLATFORM_AARCH64
}


unsigned CPUID::getCPUStepping() {
#if defined(CBANG_HWPLATFORM_INTEL)
  return getBits(getCPUSignature(), 3, 0);

#elif defined(CBANG_HWPLATFORM_AARCH64)
  return MIDR_EL1(3, 0);
#endif // CBANG_HWPLATFORM_AARCH64
}


void CPUID::getCPUCounts(uint32_t &logical, uint32_t &cores,
                         uint32_t &threads) {
  // Defaults
  logical = cores = 1;
  threads = 0;

#if defined(CBANG_HWPLATFORM_INTEL)
  // Logical core count per CPU
  cpuID(1);
  logical = EBX(23, 16);
  if (!logical) logical = 1;

  string vendor = getCPUVendor();
  if (vendor == "GenuineIntel") {
    cpuID(4); // Get DCP cache info
    cores = EAX(31, 26) + 1; // APIC IDs reserved for the package

    // Divide counts by threads/cache for i7
    uint32_t threadsPerCache = EAX(25, 14) + 1;
    if (1 < threadsPerCache) {
      cores /= threadsPerCache;
      logical /= threadsPerCache;
    }

  } else if (vendor == "AuthenticAMD" || vendor == "HygonGenuine") {
    // Number of CPU cores
    cpuID(0x80000008);
    cores = ECX(7, 0) + 1;

  } else cores = logical;

  if (!cores) cores = 1;
  if (cpuHasFeature(CPUFeature::FEATURE_HTT)) threads = logical / cores;
#elif defined(CBANG_HWPLATFORM_AARCH64)

#if defined(__linux__)
  long ncpu = sysconf(_SC_NPROCESSORS_CONF);
  if (0 < ncpu) {
    logical = cores = ncpu;
    return;
  }

  cpu_set_t cs;
  CPU_ZERO(&cs);
  sched_getaffinity(0, sizeof(cs), &cs);

  unsigned count = 0;
  for (int i = 0; i < 256; i++)
    count += CPU_ISSET(i, &cs) ? 1 : 0;

  if (!count) count = 1;

  logical = cores = count;

#elif defined(__APPLE__)
  uint32_t ncpu;
  size_t len = sizeof(ncpu);

  if (::sysctlbyname("hw.ncpu", &ncpu, &len, 0, 0) < 0) ncpu = 1;
  if (ncpu < 1) ncpu = 1;

  logical = cores = ncpu;

#else // __linux__
#error "Cannot detect number of CPUs"
#endif // __linux__

#endif // CBANG_HWPLATFORM_AARCH64
}


void CPUID::getCPUFeatureNames(set<string> &names) {
#if defined(CBANG_HWPLATFORM_INTEL)
  uint64_t features = getCPUFeatures();

  for (unsigned i = 0; i < CPUFeature::getCount(); i++)
    if (features & (1 << i))
      names.insert(CPUFeature(CPUFeature::getValue(i)).toString());

#elif defined(CBANG_HWPLATFORM_AARCH64)
  // TODO: fix the CPUID::getCPUFeatures() and use it instead of Arm-specific
  // things here.
  names.insert("FP");    // All AArch64 CPUs support floating point
  names.insert("ASIMD"); // All AArch64 CPUs support Advansed SIMD

  if (ID_AA64PFR0_EL1( 19, 16) == 1) names.insert("FPHP");
  if (ID_AA64PFR0_EL1( 23, 20) == 1) names.insert("ASIMDHP");
  if (ID_AA64ISAR0_EL1( 7,  4) == 2) names.insert("AES");
  if (ID_AA64ISAR0_EL1(11,  8) == 1) names.insert("SHA1");
  if (ID_AA64ISAR0_EL1(14, 12))      names.insert("SHA2");
  if (ID_AA64ISAR0_EL1(35, 32) == 1) names.insert("SHA3");
  if (ID_AA64ISAR0_EL1(15, 12) == 2) names.insert("SHA512");
  if (ID_AA64ISAR0_EL1(19, 16) == 1) names.insert("CRC32");
  if (ID_AA64ISAR0_EL1(23, 20) == 2) names.insert("ATOMICS");
  if (ID_AA64ISAR0_EL1(31, 28) == 1) names.insert("ASIMDRDM");
  if (ID_AA64ISAR0_EL1(39, 36) == 1) names.insert("SM3");
  if (ID_AA64ISAR0_EL1(43, 40) == 1) names.insert("SM4");
  if (ID_AA64ISAR0_EL1(47, 44) == 1) names.insert("ASIMDDP");
  if (ID_AA64ISAR0_EL1(51, 48) == 1) names.insert("FHM");
  if (ID_AA64ISAR0_EL1(55, 52) == 2) names.insert("TSX");
  if (ID_AA64ISAR0_EL1(55, 52) == 1) names.insert("TS");
  if (ID_AA64ISAR0_EL1(59, 56) == 1) names.insert("TLB");
  if (ID_AA64ISAR0_EL1(59, 56) == 2) names.insert("TLB TLBR");
  if (ID_AA64ISAR0_EL1(63, 60) == 1) names.insert("RNDR");
#endif
}


void CPUID::printCPUFeatures(ostream &stream) {
  set<string> features;
  getCPUFeatureNames(features);
  stream << String::join(features);
}
