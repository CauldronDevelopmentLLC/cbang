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

#ifdef __APPLE__

#include "MacOSString.h"

#include <cbang/SmartPointer.h>

using namespace cb;
using namespace std;


MacOSString::MacOSString(const char *s) :
  MacOSString(CFStringCreateWithCString(0, s, kCFStringEncodingUTF8)) {}


MacOSString::operator string () const {
  if (!ref) return string();

  // Try to avoid an extra copy
  const char *ptr = CFStringGetCStringPtr(ref, kCFStringEncodingUTF8);
  if (ptr) return string(ptr);

  // Must copy to buffer
  CFIndex len = 1 + CFStringGetMaximumSizeForEncoding(
    CFStringGetLength(ref), kCFStringEncodingUTF8);

  SmartPointer<char>::Array buf = new char[len];
  if (CFStringGetCString(ref, buf.get(), len, kCFStringEncodingUTF8))
    return buf.get();

  THROW("Failed to convert CFStringRef");
}

#endif // __APPLE__
