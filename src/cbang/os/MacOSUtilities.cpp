/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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
#include <sys/sysctl.h>

// The bulk of the get macOS version code comes from a
// stackoverflow thread http://bit.ly/3pqI8yw
// https://gist.github.com/kbernhagen/48e89cebac618e0b2bf535e4a1a11afa

namespace {

bool versionOK;
unsigned versions[3];


void initMacOSVersionViaSysctl(void * unused) {
  // try sysctlbyname "kern.osproductversion"; macos 10.13+
  char str[256];
  size_t size = sizeof(str);
  int ret = sysctlbyname("kern.osproductversion", str, &size, 0, 0);
  if (ret == 0) {
    int scans = sscanf(&str[0], "%u.%u.%u",
        &versions[0], &versions[1], &versions[2]);
    versionOK = (scans >= 2);
    // accept single-number version if > 0; this could happen in future
    if ((scans == 1) && (versions[0] > 0))
      versionOK = true;
  }
}


void initMacOSVersion(void * unused) {
  // don't rely on static zero init
  versionOK = false;
  versions[0] = versions[1] = versions[2] = 0;

  initMacOSVersionViaSysctl(0);
  if (versionOK) return;

  // `Gestalt()` actually gets the system version from this file.
  // `if (@available(macOS 10.x, *))` also gets the version from there.
  CFURLRef url = CFURLCreateWithFileSystemPath(
    0, CFSTR("/System/Library/CoreServices/SystemVersion.plist"),
    kCFURLPOSIXPathStyle, false);
  if (!url) return;

  CFReadStreamRef readStr = CFReadStreamCreateWithFile(0, url);
  CFRelease(url);
  if (!readStr) return;

  if (!CFReadStreamOpen(readStr)) {
    CFRelease(readStr);
    return;
  }

  CFErrorRef outError = 0;
  CFPropertyListRef propList = CFPropertyListCreateWithStream(
    0, readStr, 0, kCFPropertyListImmutable, 0, &outError);
  CFRelease(readStr);
  if (!propList) {
    CFShow(outError);
    CFRelease(outError);
    return;
  }

  if (CFGetTypeID(propList) != CFDictionaryGetTypeID()) {
    CFRelease(propList);
    return;
  }

  CFDictionaryRef dict = (CFDictionaryRef)propList;
  CFTypeRef ver = CFDictionaryGetValue(dict, CFSTR("ProductVersion"));
  if (ver) CFRetain(ver);
  CFRelease(dict);
  if (!ver) return;

  if (CFGetTypeID(ver) != CFStringGetTypeID()) {
    CFRelease(ver);
    return;
  }

  CFStringRef verStr = (CFStringRef)ver;
  // `1 +` for the terminating NUL (\0) character
  CFIndex size = 1 + CFStringGetMaximumSizeForEncoding(
      CFStringGetLength(verStr), kCFStringEncodingASCII);
  // `calloc` initializes the memory with all zero (all \0)
  char * cstr = (char *)calloc(1, size);
  if (!cstr) {
    CFRelease(verStr);
    return;
  }

  CFStringGetBytes(verStr, CFRangeMake(0, CFStringGetLength(verStr)),
                   kCFStringEncodingASCII, '?', false, (UInt8 *)cstr, size, 0);
  CFRelease(verStr);

  int scans = sscanf(cstr, "%u.%u.%u",
      &versions[0], &versions[1], &versions[2]);
  // There may only be two values, but only one is probably wrong.
  versionOK = (scans >= 2);
  // accept single-number version if > 0; this could happen in future
  if ((scans == 1) && (versions[0] > 0))
    versionOK = true;
  free(cstr);
}


} // anonymous namespace

using namespace std;

namespace cb {
  namespace MacOSUtilities {

    bool getMacOSVersions(
        unsigned *_Nullable outMajor,
        unsigned *_Nullable outMinor,
        unsigned *_Nullable outPatch) {
      static dispatch_once_t onceToken; // MUST be static
      dispatch_once_f(&onceToken, 0, &initMacOSVersion);
      if (versionOK) {
        if (outMajor) *outMajor = versions[0];
        if (outMinor) *outMinor = versions[1];
        if (outPatch) *outPatch = versions[2];
      }
      return versionOK;
    }

    cb::Version getMacOSVersion() {
      // NOTE: this deliberately never throws an exception
      unsigned major = 0;
      unsigned minor = 0;
      unsigned release = 0;
      bool ok = getMacOSVersions(&major, &minor, &release);
      if (!ok) return Version(0, 0, 0);
      if (major > 255) {
        major = minor = release = 255;
      } else if (minor > 255) {
        minor = release = 255;
      } else if (release > 255) {
        release = 255;
      }
      return Version((uint8_t)major, (uint8_t)minor, (uint8_t)release);
    }

    const string toString(const _Nullable CFStringRef cfstr) {
      if (!cfstr)
          return string();
      // CFStringGetCString requires an 8-bit encoding
      CFStringEncoding encoding = kCFStringEncodingUTF8;
      const char *str0 = CFStringGetCStringPtr(cfstr, encoding);
      if (str0)
          return string(str0);
      CFIndex len = 1 +
        CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfstr), encoding);
      string myString;
      char *buf = new char[len];
      if (buf) {
          if (CFStringGetCString(cfstr, buf, len, encoding))
              myString = buf;
          delete[] buf;
      }
      return myString;
    }

  }
}

#endif // __APPLE__
