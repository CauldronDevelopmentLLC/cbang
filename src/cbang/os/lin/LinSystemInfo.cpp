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

#include "LinSystemInfo.h"

#include <cbang/Catch.h>
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

#ifdef __linux__
#include <sched.h>
#endif

namespace {
  string get_proxy_var(const char *name) {
    auto value = SystemUtilities::getenv(String::toLower(name));
    if (!value) value = SystemUtilities::getenv(String::toUpper(name));
    return String::trim(string(value ? value : ""));
  }


  string readSys(const string &path) {
    try {
      if (SystemUtilities::exists(path))
        return String::trim(SystemUtilities::read(path));
    } CATCH_DEBUG(5);

    return "";
  }


  // Parse a Linux CPU list, e.g. "0-7,16,18-19"
  set<unsigned> parseCPUList(const string &s) {
    set<unsigned> cpus;
    vector<string> ranges;
    String::tokenize(s, ranges, ",");

    for (auto &range: ranges) {
      auto dash  = range.find('-');
      auto first = String::parseU32(range.substr(0, dash), true);
      auto last  = dash == string::npos ? first :
        String::parseU32(range.substr(dash + 1), true);

      for (auto cpu = first; cpu <= last; cpu++) cpus.insert(cpu);
    }

    return cpus;
  }


  set<unsigned> readOnlineCPUs() {
    return parseCPUList(readSys("/sys/devices/system/cpu/online"));
  }


  // Online CPUs sharing a physical core with the given CPU, including itself
  set<unsigned> readCoreCPUs(unsigned cpu, const set<unsigned> &online) {
    auto siblings = parseCPUList(readSys("/sys/devices/system/cpu/cpu" +
      String(cpu) + "/topology/thread_siblings_list"));

    set<unsigned> core;
    for (auto sibling: siblings)
      if (online.count(sibling)) core.insert(sibling);

    return core;
  }


  map<unsigned, double> readPerCPU(const set<unsigned> &cpus,
                                   const string &file) {
    map<unsigned, double> perf;

    for (auto cpu: cpus) {
      string value =
        readSys("/sys/devices/system/cpu/cpu" + String(cpu) + "/" + file);
      if (value.empty()) return {};
      perf[cpu] = String::parseDouble(value, true);
    }

    return perf;
  }
}


uint32_t LinSystemInfo::getCPUCount() const {
  long cpus = sysconf(_SC_NPROCESSORS_ONLN);
  return cpus < 1 ? 1 : cpus;
}


set<unsigned> LinSystemInfo::getAvailableCPUs() const {
  set<unsigned> cpus;

#ifdef __linux__
  // Fails if the affinity mask does not fit in cpu_set_t
  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);

  if (!sched_getaffinity(0, sizeof(cpuSet), &cpuSet))
    for (unsigned cpu = 0; cpu < CPU_SETSIZE; cpu++)
      if (CPU_ISSET(cpu, &cpuSet)) cpus.insert(cpu);
#endif

  return cpus;
}


vector<set<unsigned>> LinSystemInfo::getCPUPerformanceLevels() const {
  try {
    auto online = readOnlineCPUs();
    if (online.empty()) return {};

    // Intel hybrid CPUs have a separate PMU for each core type, fastest first
    vector<set<unsigned>> levels;
    set<unsigned> covered;

    for (auto name: {"cpu_core", "cpu_atom", "cpu_lowpower"}) {
      auto cpus =
        parseCPUList(readSys("/sys/devices/" + string(name) + "/cpus"));
      if (cpus.empty()) continue;

      for (auto cpu: cpus)
        if (!online.count(cpu) || !covered.insert(cpu).second) return {};

      levels.push_back(cpus);
    }

    if (!levels.empty()) {
      if (covered != online) return {}; // Incomplete
      return levels;
    }

    // ARM and RISC-V report relative CPU capacity
    auto perf = readPerCPU(online, "cpu_capacity");

    // Otherwise, compare max CPU frequencies, e.g. AMD Zen 5 & Zen 5c
    if (perf.empty()) perf = readPerCPU(online, "cpufreq/cpuinfo_max_freq");

    return clusterCPUs(perf);
  } CATCH_WARNING;

  return {};
}


vector<set<unsigned>> LinSystemInfo::getCPUCoreThreads() const {
  try {
    auto online = readOnlineCPUs();
    if (online.empty()) return {};

    vector<set<unsigned>> cores;
    set<unsigned> covered;

    for (auto cpu: online) {
      if (covered.count(cpu)) continue;

      auto core = readCoreCPUs(cpu, online);
      if (!core.count(cpu)) return {};

      // All siblings must agree, otherwise the topology is changing
      for (auto sibling: core)
        if (readCoreCPUs(sibling, online) != core) return {};

      for (auto sibling: core)
        if (!covered.insert(sibling).second) return {}; // Overlapping cores

      cores.push_back(core);
    }

    return cores;
  } CATCH_WARNING;

  return {};
}


uint64_t LinSystemInfo::getMemoryInfo(memory_info_t type) const {
  const char *search;
  switch (type) {
  case MEM_INFO_TOTAL:  search = "MemTotal:";     break;
  case MEM_INFO_FREE:   search = "MemFree:";      break;
  case MEM_INFO_SWAP:   search = "SwapFree:";     break;
  case MEM_INFO_USABLE: search = "MemAvailable:"; break;
  default: THROW("Unsupported memory info type: " << type);
  }

  try {
    auto path = "/proc/meminfo";
    if (SystemUtilities::exists(path)) {
      auto f = SystemUtilities::iopen(path);
      string   key;
      uint64_t kb;

      while (*f >> key >> kb) {
        if (key == search) return kb * 1024;
        f->ignore(256, '\n');
      }
    }
  } CATCH_DEBUG(1);

  struct sysinfo info;

  if (!sysinfo(&info)) {
    auto unit = info.mem_unit;
    switch (type) {
    case MEM_INFO_TOTAL:  return uint64_t(info.totalram)                 * unit;
    case MEM_INFO_FREE:   return uint64_t(info.freeram)                  * unit;
    case MEM_INFO_SWAP:   return uint64_t(info.freeswap)                 * unit;
    case MEM_INFO_USABLE: return uint64_t(info.freeram + info.bufferram) * unit;
    }
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


string LinSystemInfo::getMachineID() const {
  if (SystemUtilities::exists("/etc/machine-id"))
    return String::trim(SystemUtilities::read("/etc/machine-id"));

  THROW("Machine ID not available");
}


URI LinSystemInfo::getProxy(const URI &uri) const {
  string proxy;

  // Check proxy vars
  if (uri.getScheme() == "https") proxy = get_proxy_var("https_proxy");
  if (proxy.empty()) proxy = get_proxy_var("http_proxy");

  // No proxy
  if (proxy.empty()) return URI();

  // Check no_proxy
  string noProxy = get_proxy_var("no_proxy");
  if (!noProxy.empty()) {
    vector<string> tokens;
    String::tokenize(noProxy, tokens, ",");
    for (auto token: tokens)
      if (matchesProxyPattern(token, uri)) return URI();
  }

  return URI(proxy);
}
