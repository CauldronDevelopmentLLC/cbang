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

#include "LinSystemInfo.h"

#include <cbang/os/Thread.h>
#include <cbang/os/SystemUtilities.h>

using namespace cb;
using namespace std;

#if defined(__FreeBSD__)
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/sysctl.h>
#include <sys/ucred.h>

#else
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#endif

#include <unistd.h>


namespace {
  bool isLinuxThreads() {
    struct TestThread : public Thread {
      unsigned pid;
      TestThread() : pid(0) {}
      void run() override {pid = SystemUtilities::getPID();}
    };

    TestThread t;
    t.start();
    t.join();

    // LinuxThreads has a different PID for each thread
    return t.pid != SystemUtilities::getPID();
  }
}


uint32_t LinSystemInfo::getCPUCount() const {
  long cpus = sysconf(_SC_NPROCESSORS_ONLN);
  return cpus < 1 ? 1 : cpus;
}


ThreadsType LinSystemInfo::getThreadsType() {
  if (!threadsType)
    threadsType = isLinuxThreads() ? LINUX_THREADS : POSIX_THREADS;

  return threadsType;
}


uint64_t LinSystemInfo::getMemoryInfo(memory_info_t type) const {
  struct sysinfo info;

  if (!sysinfo(&info))
    switch (type) {
    case MEM_INFO_TOTAL: return (uint64_t)(info.totalram * info.mem_unit);
    case MEM_INFO_FREE:  return (uint64_t)(info.freeram  * info.mem_unit);
    case MEM_INFO_SWAP:  return (uint64_t)(info.freeswap * info.mem_unit);
    case MEM_INFO_USABLE:
      return (uint64_t)((info.freeram + info.bufferram + info.freeswap) *
                        info.mem_unit);
    }

  return 0;
}


Version LinSystemInfo::getOSVersion() const {
  struct utsname i;

  uname(&i);
  string release = i.release;
  size_t dot     = release.find('.');
  uint8_t major  = String::parseU32(release.substr(0, dot));
  uint8_t minor  = String::parseU32(release.substr(dot + 1));

  return Version(major, minor);
}
