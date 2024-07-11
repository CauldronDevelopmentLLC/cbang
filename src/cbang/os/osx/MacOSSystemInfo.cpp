/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "MacOSSystemInfo.h"
#include "MacOSString.h"

#include <cbang/hw/CPUInfo.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <IOKit/IOKitLib.h>
#include <SystemConfiguration/SystemConfiguration.h>

using namespace cb;
using namespace std;


namespace {
  get_ref(CFDictionaryRef dict, const string &key) {
    CFDictionaryGetValue(dict, CFSTR(key.c_str()));
  }


  int64_t get_int(
    CFDictionaryRef dict, const string &key, uint64_t defaultValue = 0) {
    auto ref = get_ref(dict, key);
    int64_t value = 0;
    if (ref && CFNumberGetValue(ref, kCFNumberSInt64Type, &value))
      return value;
    return defaultValue;
  }


  bool get_bool(CFDictionaryRef dict, const string &key) {
    return get_int(dict, key);
  }


  string get_str(CFDictionaryRef dict, const string &key) {
    auto ref = get_ref(dict, key);
    return ref ? MacOSString::convert(ref) : string();
  }
}


MacOSSystemInfo::MacOSSystemInfo() {
  unsigned ver[3] = {0, 0, 0};

  if (versionBySysCtrl(ver) || versionByPList(ver))
    osVersion = Version((uint8_t)ver[0], (uint8_t)ver[1], (uint8_t)ver[2]);
}


uint32_t MacOSSystemInfo::getCPUCount() const {
  int nm[2];
  size_t length = 4;
  uint32_t count = 0;

  nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
  sysctl(nm, 2, &count, &length, 0, 0);

  if (!count) {
    nm[1] = HW_NCPU;
    sysctl(nm, 2, &count, &length, 0, 0);
  }

  return count ? count : 1;
}


uint32_t MacOSSystemInfo::getPerformanceCPUCount() const {
  int32_t pcount = 0;
  size_t len = sizeof(pcount);

  // macOS 12+
  if (!::sysctlbyname("hw.perflevel0.logicalcpu", &pcount, &len, 0, 0))
    if (0 < pcount) return pcount;

#ifdef __aarch64__
  // macOS 11 ARM; only four possibilities
  string brand = CPUInfo::create()->getBrand();

  if (brand == "Apple M1")     return 4;
  if (brand == "Apple M1 Max") return 8;
  if (brand == "Apple M1 Pro") return getCPUCount() - 2; // 6 or 8
#endif // __aarch64__

  return 0;
}


uint64_t MacOSSystemInfo::getMemoryInfo(memory_info_t type) const {
  if (type == MEM_INFO_TOTAL) {
    int64_t memory;
    int mib[2];
    size_t length = 2;

    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    length = sizeof(int64_t);

    if (!sysctl(mib, 2, &memory, &length, 0, 0)) return memory;

  } else {
    vm_size_t page;
    vm_statistics_data_t stats;
    mach_msg_type_number_t count = sizeof(stats) / sizeof(natural_t);
    mach_port_t port = mach_host_self();

    if (host_page_size(port, &page) == KERN_SUCCESS &&
        host_statistics(port, HOST_VM_INFO, (host_info_t)&stats, &count) ==
        KERN_SUCCESS)
      switch (type) {
      case MEM_INFO_TOTAL: break; // Handled above
      case MEM_INFO_FREE: return (uint64_t)(stats.free_count * page);
      case MEM_INFO_SWAP: return 0; // Not sure how to get this on OSX
      case MEM_INFO_USABLE:
        // An IBM article says this should be free + inactive + file-backed
        // pages.  No idea how to get at file-backed pages.
        return (uint64_t)((stats.free_count + stats.inactive_count) * page);
      }
  }

  return 0;
}


string MacOSSystemInfo::getMachineID() const {
  io_registry_entry_t root =
    IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");

  MacOSString uuid((CFStringRef)IORegistryEntryCreateCFProperty(
                     root, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0));
  IOObjectRelease(root);

  return uuid;
}


// MacOS version code originates from a stackoverflow thread.
//
//   https://stackoverflow.com/questions/11072804
//   https://gist.github.com/kbernhagen/48e89cebac618e0b2bf535e4a1a11afa
bool MacOSSystemInfo::versionBySysCtrl(unsigned ver[3]) {
  // Try sysctlbyname "kern.osproductversion"; macos 10.13+
  char str[256];
  size_t size = sizeof(str);

  return !sysctlbyname("kern.osproductversion", str, &size, 0, 0) &&
    0 < sscanf(&str[0], "%u.%u.%u", &ver[0], &ver[1], &ver[2]);
}


bool MacOSSystemInfo::versionByPList(unsigned ver[3]) {
  // `Gestalt()` actually gets the system version from this file.
  // `if (@available(macOS 10.x, *))` also gets the version from there.
  MacOSRef<CFURLRef> url(CFURLCreateWithFileSystemPath(
    0, CFSTR("/System/Library/CoreServices/SystemVersion.plist"),
    kCFURLPOSIXPathStyle, false));
  if (!url) return false;

  MacOSRef<CFReadStreamRef> stream(CFReadStreamCreateWithFile(0, url));
  if (!stream || !CFReadStreamOpen(stream)) return false;

  MacOSRef<CFPropertyListRef> props(CFPropertyListCreateWithStream(
    0, stream, 0, kCFPropertyListImmutable, 0, 0));
  if (!props || CFGetTypeID(props) != CFDictionaryGetTypeID())
    return false;

  CFDictionaryRef dict = (CFDictionaryRef)(CFPropertyListRef)props;
  CFTypeRef verStr =
    CFDictionaryGetValue(dict, CFSTR("ProductVersion"));
  if (!verStr) return false;
  if (CFGetTypeID(verStr) != CFStringGetTypeID()) return false;

  CFRetain(verStr);
  string vStr = MacOSString((CFStringRef)verStr);

  return 0 < sscanf(vStr.c_str(), "%u.%u.%u", &ver[0], &ver[1], &ver[2]);
}


URI MacOSSystemInfo::getProxy(const URI &uri) const {
  CFDictionaryRef proxies = SCDynamicStoreCopyProxies(0);
  if (!proxies) return URI();

  // Check ExceptionsList
  auto list = (CFArrayRef)get_ref(proxies, "ExceptionsList");

  if (list)
    for (int i = 0; i < CFArrayGetCount(list); i++) {
      auto ref = (CFStringRef)CFArrayGetValueAtIndex(list, i);
      if (matchesProxyPattern(MacOSString::convert(ref), uri))
        return URI();
    }

  // Check ExcludeSimpleHostnames
  if (get_bool(proxies, "ExcludeSimpleHostnames") &&
    uri.getHost() == "127.0.0.1") return URI();

  // NOTE, ProxyAutoDiscoveryEnable (wpad), ProxyAutoConfigEnable (pac) and
  // SOCKS are not supported

  // Check proxy is enabled for scheme
  string scheme = String::toUpper(url.getScheme());
  if (!get_bool(proxies, scheme + "Enable")) return URI();

  // Get proxy
  auto port = get_int(proxies, scheme + "Port", 80);
  auto host = get_str(proxies, scheme + "Proxy");
  return host.empty() ? URI() : URI("http://" + host + ":" + String(port));
}
