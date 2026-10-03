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
#include <cmath>

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


SystemInfo::cpu_affinity_capability_t
LinSystemInfo::getCPUAffinityCapability() const {
#ifdef __linux__
  // sched_getaffinity() fails if the fixed cpu_set_t is too small to
  // represent the kernel's CPU affinity mask.  Do not advertise hard
  // affinity when cbang cannot represent that mask.
  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);

  if (sched_getaffinity(0, sizeof(cpuSet), &cpuSet))
    return CPU_AFFINITY_NONE;

  return CPU_AFFINITY_HARD;
#else
  return CPU_AFFINITY_NONE;
#endif
}


set<unsigned> LinSystemInfo::getAvailableCPUs() const {
#ifdef __linux__
  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);
  if (sched_getaffinity(0, sizeof(cpuSet), &cpuSet)) return {};

  set<unsigned> cpus;
  for (unsigned cpu = 0; cpu < CPU_SETSIZE; cpu++)
    if (CPU_ISSET(cpu, &cpuSet)) cpus.insert(cpu);

  return cpus;
#else
  return SystemInfo::getAvailableCPUs();
#endif
}


set<unsigned> LinSystemInfo::getPerformanceCPUs() const {
  try {
    auto online = parseCPUList(readSys("/sys/devices/system/cpu/online"));
    if (online.size() < 2) return {};

    // Intel hybrid CPUs have separate PMUs for P-cores and E-cores
    auto pCores = parseCPUList(readSys("/sys/devices/cpu_core/cpus"));
    auto eCores = parseCPUList(readSys("/sys/devices/cpu_atom/cpus"));
    if (!pCores.empty() && !eCores.empty()) return pCores;

    // ARM and RISC-V report relative CPU capacity
    auto perf = readPerCPU(online, "cpu_capacity");

    // Otherwise, compare max CPU frequencies, e.g. AMD Zen 5 & Zen 5c
    if (perf.empty()) perf = readPerCPU(online, "cpufreq/cpuinfo_max_freq");

    return selectFastestCPUs(perf);
  } CATCH_WARNING;

  return {};
}


vector<set<unsigned>> LinSystemInfo::getCPUPerformanceLevels() const {
  try {
    auto online = parseCPUList(readSys("/sys/devices/system/cpu/online"));
    if (online.empty()) return {};
    if (online.size() == 1) return {online};

    // Intel hybrid PMUs identify the core classes directly.  Current kernels
    // may expose cpu_core, cpu_atom and cpu_lowpower, so do not assume there
    // are exactly two classes.  Only use the PMU classification when the
    // classes are disjoint and together cover every online CPU.
    vector<set<unsigned>> intel;
    for (auto name: {"cpu_core", "cpu_atom", "cpu_lowpower"}) {
      auto cpus = parseCPUList(
        readSys("/sys/devices/" + string(name) + "/cpus"));
      if (!cpus.empty()) intel.push_back(cpus);
    }

    if (!intel.empty()) {
      set<unsigned> covered;
      bool valid = true;

      for (auto &cpus: intel)
        for (auto cpu: cpus)
          if (!online.count(cpu) || !covered.insert(cpu).second)
            valid = false;

      if (valid && covered == online) return intel;
      // Partial/contradictory PMU evidence must not become one frequency class.
      return {};
    }

    // ARM and RISC-V expose relative CPU capacity.  Capacity is an ordered
    // performance measure and may describe more than two classes.
    auto perf = readPerCPU(online, "cpu_capacity");
    if (!perf.empty()) {
      map<double, set<unsigned>> classes;
      for (auto &p: perf) {
        if (!isfinite(p.second) || p.second <= 0) return {};
        classes[p.second].insert(p.first);
      }

      vector<set<unsigned>> levels;
      for (auto it = classes.rbegin(); it != classes.rend(); it++)
        levels.push_back(it->second);
      return levels;
    }

    // Max frequency is only a fallback, e.g. AMD Zen/Zen-c.  Small frequency
    // differences can occur inside one core class, so preserve the existing
    // selectFastestCPUs() heuristic instead of treating each value as a class.
    perf = readPerCPU(online, "cpufreq/cpuinfo_max_freq");
    if (perf.size() != online.size()) return {};
    for (auto &p: perf)
      if (!isfinite(p.second) || p.second <= 0) return {};

    auto fastest = selectFastestCPUs(perf);
    // Valid complete measurements with no significant gap form one class
    // under the existing frequency heuristic.
    if (fastest.empty()) return {online};

    set<unsigned> slower;
    for (auto cpu: online)
      if (!fastest.count(cpu)) slower.insert(cpu);
    if (slower.empty()) return {online};

    return {fastest, slower};
  } CATCH_WARNING;

  return {};
}


vector<set<unsigned>> LinSystemInfo::getCPUCoreThreads() const {
  try {
    auto online = parseCPUList(readSys("/sys/devices/system/cpu/online"));
    if (online.empty()) return {};

    vector<set<unsigned>> cores;
    set<unsigned> covered;

    for (auto cpu: online) {
      if (covered.count(cpu)) continue;

      auto siblings = parseCPUList(readSys(
        "/sys/devices/system/cpu/cpu" + String(cpu) +
        "/topology/thread_siblings_list"));
      if (siblings.empty()) return {};

      // Sysfs topology may include an offline sibling.  Report only online
      // logical CPUs because these are the IDs that can actually be pinned.
      set<unsigned> core;
      for (auto sibling: siblings)
        if (online.count(sibling)) core.insert(sibling);

      if (core.empty() || !core.count(cpu)) return {};

      // Require a symmetric view from every online sibling.  This avoids
      // returning a partial or internally inconsistent topology during a
      // hotplug/topology transition.
      for (auto sibling: core) {
        auto peerSiblings = parseCPUList(readSys(
          "/sys/devices/system/cpu/cpu" + String(sibling) +
          "/topology/thread_siblings_list"));
        if (peerSiblings.empty()) return {};

        set<unsigned> peerCore;
        for (auto peer: peerSiblings)
          if (online.count(peer)) peerCore.insert(peer);
        if (peerCore != core) return {};
      }

      for (auto sibling: core)
        if (!covered.insert(sibling).second) return {}; // Overlapping cores

      cores.push_back(core);
    }

    if (covered != online) return {};
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
