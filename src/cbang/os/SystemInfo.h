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

#pragma once

#include <cbang/Exception.h>
#include <cbang/util/Version.h>
#include <cbang/net/SockAddr.h>
#include <cbang/net/URI.h>

#include <cstdint>
#include <set>
#include <map>
#include <vector>


namespace cb {
  class Info;

  class SystemInfo {
    static SystemInfo *singleton;

  protected:
    SystemInfo() {}
    virtual ~SystemInfo() {}

  public:
    typedef enum {
      MEM_INFO_TOTAL,
      MEM_INFO_FREE,
      MEM_INFO_SWAP,
      MEM_INFO_USABLE,
    } memory_info_t;

    static SystemInfo &instance();

    virtual uint32_t getCPUCount() const = 0;

    /// Logical CPUs the current process may run on.  On Linux this is the
    /// calling thread's affinity mask.  Returns an empty set if CPU affinity
    /// is not supported.
    virtual std::set<unsigned> getAvailableCPUs() const {return {};}

    /// Online logical CPUs grouped by performance level, fastest first.
    /// Returns one level if all cores are the same or an empty vector if the
    /// topology is unknown.
    virtual std::vector<std::set<unsigned>>
    getCPUPerformanceLevels() const {return {};}

    /// Online logical CPUs grouped by physical core.  Returns an empty vector
    /// if the topology is unknown.
    virtual std::vector<std::set<unsigned>>
    getCPUCoreThreads() const {return {};}

    /// Logical CPU indices of the fastest cores on hybrid CPUs.  Returns an
    /// empty set if all cores are the same or the topology is unknown.
    std::set<unsigned> getPerformanceCPUs() const;
    virtual uint32_t getPerformanceCPUCount() const;

    virtual uint64_t getMemoryInfo(memory_info_t type) const = 0;
    uint64_t getTotalMemory()    const {return getMemoryInfo(MEM_INFO_TOTAL);}
    uint64_t getFreeMemory()     const {return getMemoryInfo(MEM_INFO_FREE);}
    uint64_t getFreeSwapMemory() const {return getMemoryInfo(MEM_INFO_SWAP);}
    uint64_t getUsableMemory()   const {return getMemoryInfo(MEM_INFO_USABLE);}

    virtual uint64_t getFreeDiskSpace(const std::string &path);
    virtual Version getOSVersion() const = 0;
    virtual std::string getHostname() const;
    virtual std::string getMachineID() const = 0;

    virtual void getNameservers(std::vector<SockAddr> &addrs);
    virtual URI getProxy(const URI &uri) const = 0;

    static bool matchesProxyPattern(const std::string &pattern, const URI &uri);

    void add(Info &info);

  protected:
    static std::vector<std::set<unsigned>>
    clusterCPUs(const std::map<unsigned, double> &perf);
  };
}
