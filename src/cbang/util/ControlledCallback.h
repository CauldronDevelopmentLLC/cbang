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

#include "SmartFunctor.h"
#include "NonCopyable.h"
#include "LifetimeObject.h"

#include <cbang/Catch.h>

#include <functional>


namespace cb {
  template <typename... Args>
  class ControlledCallback {
  public:
    typedef std::function<void (Args...)> cb_t;

  private:
    class Impl : public NonCopyable {
    public:
      cb_t cb;
      std::function<void()> completedCB;

      Impl(cb_t cb) : cb(cb) {}
      virtual ~Impl() {CBANG_TRY_CATCH_ERROR(cancel());}

      void setCompletedCallback(std::function<void()> cb) {completedCB = cb;}
      void cancel() {if (cb) {cb = 0; onCompleted();}}
      virtual void onCompleted() {if (completedCB) completedCB();}

      void operator()(Args... args) {
        if (!cb) return;
        SmartFunctor<Impl> _(this, &Impl::cancel);
        cb(std::forward<Args>(args)...);
      }
    };


    class Lifetime : public LifetimeObject {
      Impl &cb;

    public:
      explicit Lifetime(Impl &cb) : cb(cb) {
        cb.setCompletedCallback([this] {endOfLife();});
      }

      ~Lifetime() {if (isAlive()) cb.cancel();}
    };


    SmartPointer<Impl> impl;
    bool createdLT = false;

  public:
    template <typename CB>
    ControlledCallback(CB cb) : impl(new Impl(cb)) {}

    SmartPointer<Lifetime> createLifetime() {
      if (createdLT) CBANG_THROW("Cannot create multiple CallbackLifetimes");
      createdLT = true;
      return new Lifetime(*impl);
    }

    explicit operator bool() const {return (bool)impl->cb;}
    void operator()(Args... args) {(*impl)(std::forward<Args>(args)...);}
  };
}
