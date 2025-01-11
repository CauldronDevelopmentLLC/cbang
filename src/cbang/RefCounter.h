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

#include "Deallocators.h"

#include <atomic>
#include <string>


namespace cb {
  class RefCounted;


  /// This class is used by SmartPointer to count pointer references
  class RefCounter {
  protected:
    virtual ~RefCounter() {} // Prevent deallocation by others

  public:
    static RefCounter *getCounter(const void *ptr) {return 0;}
    static RefCounter *getCounter(const RefCounted *ptr);
    static void setCounter(const void *ptr, RefCounter *counter) {}
    static void setCounter(const RefCounted *ptr, RefCounter *counter);

    virtual bool isActive() const = 0;
    virtual unsigned getCount(bool weak) const = 0;
    virtual void incCount(bool weak) = 0;
    virtual void decCount(bool weak) = 0;
    virtual void adopted() = 0;

    static void raise(const std::string &msg);

    [[gnu::format(printf, 3, 4)]]
    void log(unsigned level, const char *fmt, ...);
  };


  class RefCounted {
    RefCounter *counter = 0;
    friend class RefCounter;
  };


  template<typename T, class DeallocT>
  class RefCounterImpl : public RefCounter {
  protected:
    T *ptr;
    std::atomic<unsigned> count;
    std::atomic<unsigned> weakCount;

    RefCounterImpl(T *ptr) : ptr(ptr), count(0), weakCount(0) {}

  public:
    static unsigned trace;

    static RefCounter *getCounter(T *ptr) {
      RefCounter *counter = RefCounter::getCounter(ptr);

      if (!counter) {
        counter = new RefCounterImpl<T, DeallocT>(ptr);
        setCounter(ptr, counter);
      }

      return counter;
    }

    // From RefCounter
    bool isActive() const override {return ptr;}

    unsigned getCount(bool weak) const override {
      return weak ? weakCount : count;
    }

    void incCount(bool weak) override {
      unsigned c = getCount(weak);

      while (!(weak ? weakCount : count).compare_exchange_weak(c, c + 1))
        continue;

      log(trace, "incCount() count=%u", c + 1);
    }

    void decCount(bool weak) override {
      unsigned c = getCount(weak);

      if (!c) raise("Already zero!");

      while (!(weak ? weakCount : count).compare_exchange_weak(c, c - 1))
        if (!c) raise("Already zero!");

      log(trace, "decCount() count=%u", c - 1);

      if (c == 1) {
        if (weak && count == 0) delete this;

        if (!weak) {
          T *_ptr = ptr;
          if (weakCount == 0) delete this;
          else ptr = 0; // Deactivate
          if (_ptr) DeallocT::dealloc(_ptr);
        }
      }
    }

    void adopted() override {
      if (1 < getCount(true) + getCount(false))
        raise("Can't adopt pointer with multiple references!");
      setCounter(ptr, 0);
      delete this;
    }
  };


  template<typename T, class DeallocT>
  unsigned RefCounterImpl<T, DeallocT>::trace = 0;


  class RefCounterPhonyImpl : public RefCounter {
    static RefCounterPhonyImpl singleton;
    RefCounterPhonyImpl() {}

  public:
    static RefCounter *getCounter(const void *ptr) {return &singleton;}

    // From RefCounter
    bool isActive() const override {return true;}
    unsigned getCount(bool weak) const override {return 1;}
    void incCount(bool weak) override {}
    void decCount(bool weak) override {}
    void adopted() override {}
  };
}
