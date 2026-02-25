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

#pragma once

#include <functional>

namespace cb {
  // Forward declaration required by callable_args trait below
  template <typename T, typename... Args> class WeakCallback;

  // Trait to extract Args... from any callable: lambda, std::function, or
  // function pointer.  Specializations cover the common cases.
  template <typename F> struct callable_args;

  // std::function<void(Args...)>
  template <typename... Args>
  struct callable_args<std::function<void(Args...)>> {
    template <typename T>
    using weak_cb = WeakCallback<T, Args...>;
  };

  // Non-const lambda / functor: deduce from operator()
  template <typename F>
  struct callable_args : callable_args<
    decltype(std::function(std::declval<F>()))> {};


  template <typename T, typename... Args>
  class WeakCallback {
  public:
    typedef std::function<void (Args...)> cb_t;

  protected:
    typename SmartPointer<T>::Weak ptr;
    cb_t cb;

  public:
    WeakCallback(T *ptr, const cb_t &cb) : ptr(ptr), cb(cb) {}

    WeakCallback(const SmartPointer<T> &ptr, const cb_t &cb) :
      ptr(ptr), cb(cb) {}

    void operator()(Args... args) const {
      // Check pointer is still alive and prevent deallocation during callback
      SmartPointer<T> strong = ptr;
      if (strong.isSet()) cb(args...);
    }
  };


  template <typename T, typename F>
  auto WeakCall(T *ptr, F &&cb) {
    using cb_t = decltype(std::function(std::forward<F>(cb)));
    return typename callable_args<cb_t>::template weak_cb<T>(
      ptr, std::function(std::forward<F>(cb)));
  }

  template <typename T, typename F>
  auto WeakCall(const SmartPointer<T> &ptr, F &&cb) {
    using cb_t = decltype(std::function(std::forward<F>(cb)));
    return typename callable_args<cb_t>::template weak_cb<T>(
      ptr, std::function(std::forward<F>(cb)));
  }
}