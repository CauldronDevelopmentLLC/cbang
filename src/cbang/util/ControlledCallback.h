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
      cb_t cb;
      LifetimeObject *lto = 0;

    public:
      Impl(cb_t cb) : cb(cb) {
        CBANG_LOG_DEBUG(4, CBANG_FUNC << "() " << this);
      }

      ~Impl() {
        CBANG_LOG_DEBUG(4, CBANG_FUNC << "() " << this);
        cancel();
      }

      void setLTO(LifetimeObject *lto) {
        if (this->lto) CBANG_THROW("LifetimeObject already set");
        this->lto = lto;
      }

      void cancel() {
        CBANG_LOG_DEBUG(4, CBANG_FUNC << "() " << this << " active=" << !!cb
                        << " lto=" << lto);
        if (cb) {
          cb = 0;
          if (lto) lto->endOfLife();
        }
        lto = 0;
      }

      explicit operator bool() const {return (bool)cb;}

      void operator()(Args... args) {
        CBANG_LOG_DEBUG(4, CBANG_FUNC << "() " << this << " active=" << !!cb);
        if (!cb) return;
        CBANG_TRY_CATCH_ERROR(cb(args...));
        cancel();
      }
    };


    class Lifetime : public LifetimeObject {
      SmartPointer<Impl> impl;

    public:
      Lifetime(const SmartPointer<Impl> &impl) : impl(impl) {
        CBANG_LOG_DEBUG(4, CBANG_FUNC << "() " << impl.get());
        impl->setLTO(this);
      }

      ~Lifetime() {
        CBANG_LOG_DEBUG(4, CBANG_FUNC << "() " << impl.get());
        impl->cancel();
      }
    };


    SmartPointer<Impl> impl;

  public:
    template <typename CB>
    ControlledCallback(CB cb) : impl(new Impl(cb)) {}

    SmartPointer<Lifetime> createLifetime() {return new Lifetime(impl);}

    explicit operator bool() const {return (bool)*impl;}
    void operator()(Args... args) {(*impl)(args...);}
  };
}
