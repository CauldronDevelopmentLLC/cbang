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

#ifdef __APPLE__

#include "MacOSUtilities.h"
#include "MacOSString.h"

#include <sys/sysctl.h>

#include <CoreFoundation/CoreFoundation.h>

using namespace cb;
using namespace std;


// MacOS version code originates from a stackoverflow thread.
//
//   https://stackoverflow.com/questions/11072804
//   https://gist.github.com/kbernhagen/48e89cebac618e0b2bf535e4a1a11afa

namespace cb {
  namespace MacOSUtilities {
    Version osVersion;


    bool versionBySysCtrl(unsigned ver[3]) {
      // Try sysctlbyname "kern.osproductversion"; macos 10.13+
      char str[256];
      size_t size = sizeof(str);

      return !sysctlbyname("kern.osproductversion", str, &size, 0, 0) &&
        0 < sscanf(&str[0], "%u.%u.%u", &ver[0], &ver[1], &ver[2]);
    }


    bool versionByPList(unsigned ver[3]) {
      // `Gestalt()` actually gets the system version from this file.
      // `if (@available(macOS 10.x, *))` also gets the version from there.
      MacOSRef<CFURLRef> url = CFURLCreateWithFileSystemPath(
        0, CFSTR("/System/Library/CoreServices/SystemVersion.plist"),
        kCFURLPOSIXPathStyle, false);
      if (!url) return false;

      MacOSRef<CFReadStreamRef> stream = CFReadStreamCreateWithFile(0, url);
      if (!stream || !CFReadStreamOpen(stream)) return false;

      MacOSRef<CFPropertyListRef> props = CFPropertyListCreateWithStream(
        0, stream, 0, kCFPropertyListImmutable, 0, 0);
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


    void initVersion(void *unused) {
      unsigned ver[3] = {0, 0, 0};

      if (versionBySysCtrl(ver) || versionByPList(ver))
        osVersion = Version((uint8_t)ver[0], (uint8_t)ver[1], (uint8_t)ver[2]);
    }


    const Version &getOSVersion() {
      static dispatch_once_t once; // MUST be static
      dispatch_once_f(&once, 0, &initVersion);

      return osVersion;
    }
  }
}

#endif // __APPLE__
