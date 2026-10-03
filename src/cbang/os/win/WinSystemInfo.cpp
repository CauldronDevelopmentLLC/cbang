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

#include "WinSystemInfo.h"
#include "Win32Registry.h"

#include <cbang/os/SysError.h>
#include <cbang/net/Winsock.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sysinfoapi.h>
#include <iphlpapi.h>
#include <cstddef>

#pragma comment(lib, "iphlpapi.lib")

using namespace cb;
using namespace std;


namespace {
  URI schemelessURI(const string &s) {
    return s.find("://") == string::npos ? "http://" + s : s;
  }


  bool hasRepresentableAffinityMask() {
    // WOW64 folds 64-bit affinity masks into 32 bits.  Reject any single
    // processor group that is wider than the current process can represent.
    if (GetActiveProcessorGroupCount() != 1) return false;

    auto count = GetActiveProcessorCount(0);
    return count && count <= sizeof(KAFFINITY) * 8;
  }
}


uint32_t WinSystemInfo::getCPUCount() const {
  auto cores = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
  if (cores) return (uint32_t)cores;

  // Fallback to old method
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  return sysInfo.dwNumberOfProcessors;
}


SystemInfo::cpu_affinity_capability_t
WinSystemInfo::getCPUAffinityCapability() const {
  // The current cbang affinity representation is a flat mask for one
  // processor group.  Do not advertise hard affinity when that representation
  // cannot address the machine correctly.
  return hasRepresentableAffinityMask() ?
    CPU_AFFINITY_HARD : CPU_AFFINITY_NONE;
}


set<unsigned> WinSystemInfo::getAvailableCPUs() const {
  // Keep the flat-mask limitation; reject unrepresentable WOW64 groups too.
  if (!hasRepresentableAffinityMask()) return {};

  DWORD_PTR processMask = 0, systemMask = 0;
  if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask))
    return {};

  set<unsigned> cpus;
  for (unsigned cpu = 0; cpu < sizeof(processMask) * 8; cpu++)
    if (processMask & ((DWORD_PTR)1 << cpu)) cpus.insert(cpu);

  return cpus;
}


set<unsigned> WinSystemInfo::getPerformanceCPUs() const {
  // Only machines with a single processor group are supported.  CPU indices
  // are bit positions in that group's affinity mask.
  if (!hasRepresentableAffinityMask()) return {};

  DWORD size = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, 0, &size);
  if (!size) return {};

  vector<uint8_t> buf(size);
  auto data = buf.data();
  if (!GetLogicalProcessorInformationEx(RelationProcessorCore,
      (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)data, &size)) return {};

  // Higher EfficiencyClass means higher performance
  map<unsigned, BYTE> classes;
  BYTE maxClass = 0;
  BYTE minClass = 255;

  for (DWORD offset = 0; offset < size;) {
    auto info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(data + offset);
    if (!info->Size) return {};
    offset += info->Size;

    if (info->Relationship != RelationProcessorCore) continue;
    auto &proc = info->Processor;
    if (proc.GroupCount != 1 || proc.GroupMask[0].Group) return {};

    BYTE eClass = proc.EfficiencyClass;
    if (maxClass < eClass) maxClass = eClass;
    if (eClass < minClass) minClass = eClass;

    KAFFINITY mask = proc.GroupMask[0].Mask;
    for (unsigned i = 0; i < sizeof(mask) * 8; i++)
      if (mask & ((KAFFINITY)1 << i)) classes[i] = eClass;
  }

  set<unsigned> cpus;
  if (minClass < maxClass)
    for (auto &p: classes)
      if (p.second == maxClass) cpus.insert(p.first);

  return cpus;
}


vector<set<unsigned>> WinSystemInfo::getCPUPerformanceLevels() const {
  // EfficiencyClass is ordered but is not limited to two values.  Preserve
  // every class reported by Windows instead of assuming a P/E-only topology.
  // Keep the same representable flat-mask limitation as getPerformanceCPUs().
  if (!hasRepresentableAffinityMask()) return {};

  DWORD size = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, 0, &size);
  if (!size) return {};

  vector<uint8_t> buf(size);
  auto data = buf.data();
  if (!GetLogicalProcessorInformationEx(RelationProcessorCore,
      (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)data, &size)) return {};

  map<BYTE, set<unsigned>> classes;
  set<unsigned> covered;

  for (DWORD offset = 0; offset < size;) {
    if (size - offset < offsetof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
        Processor)) return {};
    auto info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(data + offset);
    if (!info->Size || info->Size > size - offset) return {};
    offset += info->Size;

    if (info->Relationship != RelationProcessorCore) continue;
    if (info->Size < offsetof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
        Processor) + sizeof(PROCESSOR_RELATIONSHIP)) return {};
    auto &proc = info->Processor;
    if (proc.GroupCount != 1 || proc.GroupMask[0].Group) return {};

    auto &cpus = classes[proc.EfficiencyClass];
    KAFFINITY mask = proc.GroupMask[0].Mask;
    if (!mask) return {};

    for (unsigned i = 0; i < sizeof(mask) * 8; i++)
      if (mask & ((KAFFINITY)1 << i)) {
        if (!covered.insert(i).second) return {};
        cpus.insert(i);
      }
  }

  auto count = GetActiveProcessorCount(0);
  if (!count || covered.size() != count) return {};

  // Preserve a complete single class instead of conflating it with failure.

  vector<set<unsigned>> levels;
  for (auto it = classes.rbegin(); it != classes.rend(); it++)
    levels.push_back(it->second);

  return levels;
}


vector<set<unsigned>> WinSystemInfo::getCPUCoreThreads() const {
  // CPU indices used by cbang affinity are bit positions in one processor
  // group's affinity mask.  Require that the full group fit that mask.
  if (!hasRepresentableAffinityMask()) return {};

  DWORD size = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, 0, &size);
  if (!size) return {};

  vector<uint8_t> buf(size);
  auto data = buf.data();
  if (!GetLogicalProcessorInformationEx(RelationProcessorCore,
      (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)data, &size)) return {};

  vector<set<unsigned>> cores;
  set<unsigned> covered;

  for (DWORD offset = 0; offset < size;) {
    auto info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(data + offset);
    if (!info->Size) return {};
    offset += info->Size;

    if (info->Relationship != RelationProcessorCore) continue;
    auto &proc = info->Processor;
    if (proc.GroupCount != 1 || proc.GroupMask[0].Group) return {};

    set<unsigned> core;
    KAFFINITY mask = proc.GroupMask[0].Mask;
    for (unsigned i = 0; i < sizeof(mask) * 8; i++)
      if (mask & ((KAFFINITY)1 << i)) core.insert(i);

    if (core.empty()) return {};
    for (auto cpu: core)
      if (!covered.insert(cpu).second) return {}; // Overlap is inconsistent

    cores.push_back(core);
  }

  auto count = GetActiveProcessorCount(0);
  if (!count || covered.size() != count) return {};

  return cores;
}


uint64_t WinSystemInfo::getMemoryInfo(memory_info_t type) const {
  MEMORYSTATUSEX info;

  info.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&info);

  switch (type) {
  case MEM_INFO_TOTAL:
    return (uint64_t)info.ullTotalPhys;

  case MEM_INFO_FREE:
  case MEM_INFO_USABLE:
    return (uint64_t)info.ullAvailPhys;

  case MEM_INFO_SWAP:
    return info.ullAvailPageFile > info.ullAvailPhys ?
      (uint64_t)(info.ullAvailPageFile - info.ullAvailPhys) : 0;
  }

  return 0;
}


Version WinSystemInfo::getOSVersion() const {
  OSVERSIONINFO vi = {0};
  vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&vi);
  return Version((uint8_t)vi.dwMajorVersion, (uint8_t)vi.dwMinorVersion);
}


string WinSystemInfo::getMachineID() const {
  return Win32Registry::getString(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\MachineGuid");
}


void WinSystemInfo::getNameservers(vector<SockAddr> &addrs) {
  // Get buffer size
  ULONG size = 0;
  if (GetAdaptersAddresses(0, 0, 0, 0, &size) != ERROR_BUFFER_OVERFLOW)
    THROW("Failed to get AdapterAddresses buffer size");

  // Allocate buffer
  SmartPointer<uint8_t>::Array buf = new uint8_t[size]();
  auto aAddrs = (PIP_ADAPTER_ADDRESSES)buf.get();

  // Get addresses
  ULONG ret = GetAdaptersAddresses(0, 0, 0, aAddrs, &size);
  if (ret == ERROR_ADDRESS_NOT_ASSOCIATED || ret == ERROR_NO_DATA) return;
  if (ret != NO_ERROR) THROW("Failed to get AdapterAddresses");

  // Add addresses
  for (; aAddrs; aAddrs = aAddrs->Next)
    if (aAddrs->OperStatus == 1)
      for (auto p = aAddrs->FirstDnsServerAddress; p; p = p->Next)
        addrs.push_back(SockAddr(*p->Address.lpSockaddr));
}


URI WinSystemInfo::getProxy(const URI &uri) const {
  string base =
    "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";

  // Check ProxyOverride
  string noProxy = Win32Registry::getString(base + "\\ProxyOverride", "");
  vector<string> tokens;
  String::tokenize(noProxy, tokens, ";");
  for (auto token: tokens)
    if (matchesProxyPattern(token, uri)) return URI();

  // NOTE wpad, pac and SOCKS are not supported

  if (Win32Registry::getU32(base + "\\ProxyEnable", 0)) {
    string list = Win32Registry::getString(base + "\\ProxyServer", "");
    vector<string> parts;
    String::tokenize(list, parts, ";");

    string defaultProxy;
    for (auto &part: parts) {
      auto equal = part.find_first_of('=');
      if (equal == string::npos) defaultProxy = part;
      else if (part.substr(0, equal) == uri.getScheme())
        return schemelessURI(part.substr(equal + 1));
    }

    if (!defaultProxy.empty()) return schemelessURI(defaultProxy);
  }

  return URI();
}
