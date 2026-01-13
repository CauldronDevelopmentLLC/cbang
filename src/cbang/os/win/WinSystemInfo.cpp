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

#pragma comment(lib, "iphlpapi.lib")

using namespace cb;
using namespace std;


namespace {
  URI schemelessURI(const string &s) {
    return s.find("://") == string::npos ? "http://" + s : s;
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


uint64_t WinSystemInfo::getMemoryInfo(memory_info_t type) const {
  MEMORYSTATUSEX info;

  info.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&info);

  switch (type) {
  case MEM_INFO_TOTAL: return (uint64_t)info.ullTotalPhys;
  case MEM_INFO_FREE:  return (uint64_t)info.ullAvailPhys;
  case MEM_INFO_SWAP:  return (uint64_t)info.ullAvailPageFile;
  case MEM_INFO_USABLE:
    return (uint64_t)(info.ullAvailPhys + info.ullAvailPageFile);
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
