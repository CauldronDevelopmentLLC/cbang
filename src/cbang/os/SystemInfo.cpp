/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "SystemInfo.h"
#include "PowerManagement.h"
#include "SystemUtilities.h"
#include "SysError.h"

#include "win/WinSystemInfo.h"
#include "osx/MacOSSystemInfo.h"
#include "lin/LinSystemInfo.h"

#include <cbang/Info.h>
#include <cbang/SStream.h>
#include <cbang/String.h>
#include <cbang/Catch.h>

#include <cbang/hw/CPUInfo.h>
#include <cbang/log/Logger.h>
#include <cbang/util/HumanSize.h>
#include <cbang/net/Socket.h>
#include <cbang/net/Winsock.h>
#include <cbang/net/SockAddr.h>
#include <cbang/net/AddressRange.h>

#include <cbang/boost/StartInclude.h>
#include <boost/filesystem/operations.hpp>
#include <cbang/boost/EndInclude.h>


using namespace cb;
using namespace std;

namespace fs = boost::filesystem;


SystemInfo *SystemInfo::singleton = 0;


SystemInfo &SystemInfo::instance() {
  if (!singleton) {
#if defined(_WIN32)
    singleton = new WinSystemInfo;
#elif defined(__APPLE__)
    singleton = new MacOSSystemInfo;
#else
    singleton = new LinSystemInfo;
#endif
  }

  return *singleton;
}


uint64_t SystemInfo::getFreeDiskSpace(const string &path) {
  fs::space_info si;

  try {
    si = fs::space(path);

  } catch (const fs::filesystem_error &e) {
    THROW("Could not get disk space at '" << path << "': " << e.what());
  }

  return si.available;
}


string SystemInfo::getHostname() const {
  Socket::initialize(); // Windows needs this

  char name[1024];
  if (gethostname(name, 1024))
    THROW("Failed to get hostname: " << SysError());

  return name;
}


void SystemInfo::getNameservers(vector<SockAddr> &addrs) {
  string path = "/etc/resolv.conf";

#ifdef DEBUG
  // Allow override in debug mode
  const char *v = SystemUtilities::getenv("CBANG_RESOLV_CONF");
  if (v) path = v;
#endif

  if (SystemUtilities::exists(path)) {
    string data = SystemUtilities::read(path);
    vector<string> lines;
    String::tokenize(data, lines, "\n\r");

    for (string &line: lines) {
      vector<string> parts;
      String::tokenize(line, parts, " \t");

      if (1 < parts.size() && parts[0] == "nameserver")
        TRY_CATCH_ERROR(addrs.push_back(SockAddr::parse(parts[1])));
    }
  }
}


void SystemInfo::add(Info &info) {
  const char *category = "System";
  auto cpuInfo = CPUInfo::create();

  info.add(category, "CPU", cpuInfo->getBrand());
  info.add(category, "CPU ID", SSTR(
           cpuInfo->getVendor()
           << " Family "   << cpuInfo->getFamily()
           << " Model "    << cpuInfo->getModel()
           << " Stepping " << cpuInfo->getStepping()));
  info.add(category, "CPUs", String(getCPUCount()));

  info.add(category, "Memory", HumanSize(getTotalMemory()).toString() + "B");
  info.add(category, "Free Memory",
           HumanSize(getFreeMemory()).toString() + "B");

  info.add(category, "OS Version", getOSVersion().toString());

  info.add(category, "Has Battery",
           String(PowerManagement::instance().hasBattery()));
  info.add(category, "On Battery",
           String(PowerManagement::instance().onBattery()));

  try {
    info.add(category, "Hostname", getHostname());
  } catch (...) {}
}


bool SystemInfo::matchesProxyPattern(const string &pattern, const URI &uri) {
  string pat = String::trim(pattern);

  if (pat == "*")  return true;
  if (pat.empty()) return false;

  string host = uri.getHost();
  if (String::startsWith(host, "[") && String::endsWith(host, "]"))
    host = host.substr(1, host.length() - 2);

  // Try matching by IP or IP range
  if (SockAddr::isAddress(host)) {
    auto addr = SockAddr::parse(host);

    // Try AddressRange
    if (pat.find_last_of('/') != string::npos)
      try {
        return AddressRange(pat).contains(addr);
      } catch (const Exception &e) {
        return false;
      }

    // Try Address
    if (SockAddr::isAddress(pat)) {
      auto patAddr = SockAddr::parse(pat);

      // Check for port in pattern
      if (patAddr.getPort()) addr.setPort(uri.getPort());

      return patAddr == addr;
    }

    return false;
  }

  // Try matching by host

  // Remove *. or . from start of pattern
  if (!pat.empty() && pat[0] == '*') pat = pat.substr(1);
  if (!pat.empty() && pat[0] == '.') pat = pat.substr(1);

  // Check domain name match
  return host == pat || String::endsWith(host, "." + pat);
}
