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

#pragma once

#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>


namespace cb {
  template <typename T>
  class MacOSRef {

  protected:
    T ref;

  public:
    explicit MacOSRef(const MacOSRef<T> &o) {*this = o;}
    explicit MacOSRef(T ref = 0) : ref(ref) {}
    virtual ~MacOSRef() {if (ref) CFRelease(ref);}


    const T &get() const {return ref;}
    T &get() {return ref;}


    MacOSRef<T> &operator=(const T &_ref) {
      if (ref != _ref) {
        if (ref) CFRelease(ref);
        ref = _ref;
      }

      return *this;
    }


    MacOSRef<T> &operator=(const MacOSRef<T> &o) {
      if (ref != o.ref) {
        if (ref) CFRelease(ref);
        ref = o.ref;
        if (ref) CFRetain(ref);
      }

      return *this;
    }


    explicit operator bool() {return ref;}
    operator T &() {return ref;}
    operator const T &() const {return ref;}
  };
}

#endif // __APPLE__
