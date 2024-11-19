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

#pragma once

#include <functional>

namespace cb {
  template <typename T, typename... Args>
  class WeakCallback {
  public:
    typedef std::function<void (Args...)> cb_t;

  protected:
    typename SmartPointer<T>::Weak ptr;
    cb_t cb;

  public:
    WeakCallback(const typename SmartPointer<T>::Weak &ptr, const cb_t &cb) :
      ptr(ptr), cb(cb) {}
    WeakCallback(RefCounted *ptr, const cb_t &cb) : ptr(ptr), cb(cb) {}

    explicit operator bool() const {return ptr.isSet() && cb;}

    void operator()(Args... args) {
      if (ptr.isNull()) return;
      SmartPointer<T> strong = ptr; // Prevent deallocation during callback
      cb(args...);
    }
  };

  template<typename... Args>
  WeakCallback<RefCounted, Args...> WeakCall(
    RefCounted *ptr, std::function<void (Args...)> cb) {
    return WeakCallback<RefCounted, Args...>(ptr, cb);
  }
}
