/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include "BsdSystemInfo.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>

using namespace cb;


uint32_t BsdSystemInfo::getCPUCount() const {
  int mib[2];
  uint32_t cpuCount;
  size_t len = sizeof(cpuCount);

  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;

  if (sysctl(mib, 2, &cpuCount, &len, NULL, 0) == -1) return 1;

  return cpuCount;
}


uint64_t BsdSystemInfo::getMemoryInfo(memory_info_t type) const {
  int mib[2];
  uint64_t memory;
  size_t len = sizeof(memory);

  mib[0] = CTL_HW;

  switch (type) {
  case MEM_INFO_TOTAL:
    mib[1] = HW_PHYSMEM64;
    break;

  case MEM_INFO_USABLE:
    mib[1] = HW_USERMEM64;
    break;


  default: return 0;
  }

  if (sysctl(mib, 2, &memory, &len, NULL, 0) == -1) return 0;

  return memory;
}


Version BsdSystemInfo::getOSVersion() const {
  struct utsname name;
  if (uname(&name) == -1) return Version();

  return Version(name.release);
}
