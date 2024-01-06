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

#include "Deallocators.h"

#include <cbang/util/FormatCheck.h>

#include <atomic>
#include <string>


namespace cb {
  class RefCounted;


  /// This class is used by SmartPointer to count pointer references
  class RefCounter {
  protected:
    virtual ~RefCounter() {} // Prevent deallocation by others

  public:
    virtual unsigned getCount() const = 0;
    virtual void incCount() = 0;
    virtual void decCount() = 0;
    virtual void adopted() = 0;

    static void raise(const std::string &msg);

    static RefCounter *getRefPtr(const RefCounted *ref);
    static RefCounter *getRefPtr(const void *ref) {return 0;}
    void setRefPtr(const RefCounted *ref);
    void setRefPtr(const void *ref) {}
    void clearRefPtr(const RefCounted *ref);
    void clearRefPtr(const void *ref) {}

    void log(unsigned level, const char *fmt, ...) FORMAT_CHECK(printf, 3, 4);
  };


  class RefCounted {
    RefCounter *counter = 0;
    friend class RefCounter;

  public:
    unsigned getRefCount() const {return counter ? counter->getCount() : 0;}
  };


  template<typename T, class Dealloc_T = DeallocNew<T> >
  class RefCounterImpl : public RefCounter {
  protected:
    T *ptr;
    std::atomic<unsigned> count;

  public:
    static unsigned trace;

    RefCounterImpl(T *ptr) : ptr(ptr), count(0) {setRefPtr(ptr);}
    static RefCounter *create(T *ptr) {return new RefCounterImpl(ptr);}

    void release() {
      log(trace, "release()");
      T *_ptr = ptr;
      delete this;
      if (_ptr) Dealloc_T::dealloc(_ptr);
    }

    // From RefCounter
    unsigned getCount() const override {return count;}

    void incCount() override {
      unsigned c = count;

      while (!count.compare_exchange_weak(c, c + 1))
        continue;

      log(trace, "incCount() count=%u", c + 1);
    }

    void decCount() override {
      unsigned c = count;

      if (!c) raise("Already zero!");

      while (!count.compare_exchange_weak(c, c - 1))
        if (!c) raise("Already zero!");

      log(trace, "decCount() count=%u", c - 1);

      if (c == 1) release();
    }

    void adopted() override {
      if (1 < getCount())
        raise("Can't adopt pointer with multiple references!");
      clearRefPtr(ptr);
      delete this;
    }
  };


  template<typename T, class Dealloc_T>
  unsigned RefCounterImpl<T, Dealloc_T>::trace = 0;


  class RefCounterPhonyImpl : public RefCounter {
    static RefCounterPhonyImpl singleton;
    RefCounterPhonyImpl() {}

  public:
    static RefCounter *create(void *ptr) {return &singleton;}

    // From RefCounter
    unsigned getCount() const override {return 1;}
    void incCount() override {}
    void decCount() override {}
    void adopted() override {}

    static RefCounter *getRefPtr(const RefCounted *ref) {return 0;}
    using RefCounter::getRefPtr;
    void setRefPtr(const RefCounted *ref) {}
    using RefCounter::setRefPtr;
  };
}
