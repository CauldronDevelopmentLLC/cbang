/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include <cbang/os/Mutex.h>

#include <typeinfo>


namespace cb {
  class RefCounted;


  /// This class is used by SmartPointer to count pointer references
  class RefCounter {
  protected:
    virtual ~RefCounter() {} // Prevent deallocation by others

  public:
    static constexpr bool enableLogging = false;

    typedef enum {
      RC_NORMAL,
      RC_PHONY,
      RC_PROTECTED,
    } type_t;

    static bool staticIsProtected() {return false;}
    virtual bool isProtected() const {return false;}
    virtual unsigned getCount() const = 0;
    virtual void incCount() = 0;
    virtual void decCount() = 0;
    virtual void adopted() = 0;

    void log(const char *name, unsigned count);

    static void raise(const std::string &msg);

    static RefCounter *getRefPtr(const RefCounted *ref);
    static RefCounter *getRefPtr(const void *ref) {return 0;}
    void setRefPtr(const RefCounted *ref);
    void setRefPtr(const void *ref) {}
    void clearRefPtr(const RefCounted *ref);
    void clearRefPtr(const void *ref) {}
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
    unsigned count = 0;

  public:
    RefCounterImpl(T *ptr) : ptr(ptr) {setRefPtr(ptr);}
    static RefCounter *create(T *ptr) {return new RefCounterImpl(ptr);}

    void release() {
      T *_ptr = ptr;
      delete this;
      if (_ptr) Dealloc_T::dealloc(_ptr);
    }

    // From RefCounter
    unsigned getCount() const {return count;}

    void incCount() {
      count++;
#ifdef DEBUG
      log(typeid(T).name(), count);
#endif
    }

    void decCount() {
      if (!count) raise("Already zero!");
#ifdef DEBUG
      log(typeid(T).name(), count - 1);
#endif
      if (!--count) release();
    }

    void adopted() {
      if (1 < getCount())
        raise("Can't adopt pointer with multiple references!");
      clearRefPtr(ptr);
      delete this;
    }
  };


  template<typename T, class Dealloc_T = DeallocNew<T> >
  class ProtectedRefCounterImpl :
    public RefCounterImpl<T, Dealloc_T>, public Mutex {
  protected:
    typedef RefCounterImpl<T, Dealloc_T> Super_T;
    using Super_T::count;

  public:
    ProtectedRefCounterImpl(T *ptr) : Super_T(ptr) {}
    static RefCounter *create(T *ptr) {return new ProtectedRefCounterImpl(ptr);}
    static bool staticIsProtected() {return true;}

    // From RefCounterImpl
    bool isProtected() const {return true;}

    unsigned getCount() const {
      lock();
      unsigned x = count;
      unlock();
      return x;
    }

    void incCount() {
      lock();
      count++;
#ifdef DEBUG
      Super_T::log(typeid(T).name(), count);
#endif
      unlock();
    }

    void decCount() {
      lock();

      if (!count) {unlock(); Super_T::raise("Already zero!");}

#ifdef DEBUG
      Super_T::log(typeid(T).name(), count - 1);
#endif

      if (!--count) {
        unlock();
        Super_T::release();

      } else unlock();
    }
  };


  class RefCounterPhonyImpl : public RefCounter {
    static RefCounterPhonyImpl singleton;
    RefCounterPhonyImpl() {}

  public:
    static RefCounter *create(void *ptr) {return &singleton;}

    // From RefCounter
    unsigned getCount() const {return 1;}
    void incCount() {}
    void decCount() {}
    void adopted() {}

    static RefCounter *getRefPtr(const RefCounted *ref) {return 0;}
    using RefCounter::getRefPtr;
    void setRefPtr(const RefCounted *ref) {}
    using RefCounter::setRefPtr;
  };
}
