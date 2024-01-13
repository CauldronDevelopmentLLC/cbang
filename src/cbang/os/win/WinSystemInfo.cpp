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

#include "WinSystemInfo.h"

#include <cbang/os/SysError.h>
#include <cbang/os/DynamicLibrary.h>

#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>
#include <sysinfoapi.h>

using namespace cb;
using namespace std;


uint32_t WinSystemInfo::getCPUCount() const {
  // Count active CPU cores
  SysError::clear();
  DWORD len = 0;
  GetLogicalProcessorInformation(0, &len);
  if (SysError::get() == ERROR_INSUFFICIENT_BUFFER) {
    typedef SYSTEM_LOGICAL_PROCESSOR_INFORMATION info_t;
    unsigned count = (unsigned)len / sizeof(info_t);
    SmartPointer<info_t>::Array buf = new info_t[count];

    if (!GetLogicalProcessorInformation(buf.get(), &len)) {
      uint32_t cores = 0;
      for (unsigned i = 0; i < count; i++)
        if (buf[i].Relationship == RelationProcessorCore)
          cores++;

      if (cores) return cores;
    }
  }

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
  DynamicLibrary ntdll("ntdll.dll");
  typedef LONG (WINAPI *func_t)(PRTL_OSVERSIONINFOW);
  auto func = (func_t)ntdll.getSymbol("RtlGetVersion");

  if (func) {
    RTL_OSVERSIONINFOW vi = {0};
    vi.dwOSVersionInfoSize = sizeof(vi);
    if (!func(&vi))
      return Version((uint8_t)vi.dwMajorVersion, (uint8_t)vi.dwMinorVersion);
  }

  THROW("Failed to get Windows version");
}
