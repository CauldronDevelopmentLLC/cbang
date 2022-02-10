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

#include "CPUInfoAArch64.h"
#include "SystemInfo.h"

using namespace cb;
using namespace std;


CPUInfoAArch64::CPUInfoAArch64() {
  vendor    = getCPUVendor();
  brand     = getCPUBrand();
  signature = getCPUSignature();
  family    = getCPUFamily();
  model     = getCPUModel();
  stepping  = getCPUStepping();
  physical  = SystemInfo::instance().getCPUCount();
  threads   = 1;

  features.insert("FP");    // All AArch64 CPUs support floating point
  features.insert("ASIMD"); // All AArch64 CPUs support Advansed SIMD

  if (ID_AA64PFR0_EL1( 19, 16) == 1) features.insert("FPHP");
  if (ID_AA64PFR0_EL1( 23, 20) == 1) features.insert("ASIMDHP");
  if (ID_AA64ISAR0_EL1( 7,  4) == 2) features.insert("AES");
  if (ID_AA64ISAR0_EL1(11,  8) == 1) features.insert("SHA1");
  if (ID_AA64ISAR0_EL1(14, 12))      features.insert("SHA2");
  if (ID_AA64ISAR0_EL1(35, 32) == 1) features.insert("SHA3");
  if (ID_AA64ISAR0_EL1(15, 12) == 2) features.insert("SHA512");
  if (ID_AA64ISAR0_EL1(19, 16) == 1) features.insert("CRC32");
  if (ID_AA64ISAR0_EL1(23, 20) == 2) features.insert("ATOMICS");
  if (ID_AA64ISAR0_EL1(31, 28) == 1) features.insert("ASIMDRDM");
  if (ID_AA64ISAR0_EL1(39, 36) == 1) features.insert("SM3");
  if (ID_AA64ISAR0_EL1(43, 40) == 1) features.insert("SM4");
  if (ID_AA64ISAR0_EL1(47, 44) == 1) features.insert("ASIMDDP");
  if (ID_AA64ISAR0_EL1(51, 48) == 1) features.insert("FHM");
  if (ID_AA64ISAR0_EL1(55, 52) == 2) features.insert("TSX");
  if (ID_AA64ISAR0_EL1(55, 52) == 1) features.insert("TS");
  if (ID_AA64ISAR0_EL1(59, 56) == 1) features.insert("TLB");
  if (ID_AA64ISAR0_EL1(59, 56) == 2) features.insert("TLB TLBR");
  if (ID_AA64ISAR0_EL1(63, 60) == 1) features.insert("RNDR");

  registers["MIDR_EL1"]         = MIDR_EL1();
  registers["ID_AA64ISAR0_EL1"] = ID_AA64ISAR0_EL1();
  registers["ID_AA64ISAR1_EL1"] = ID_AA64ISAR1_EL1();
  registers["ID_AA64PFR0_EL1"]  = ID_AA64PFR0_EL1();
}
