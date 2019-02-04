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


namespace cb {
  /// This class is used by SmartPointer to count pointer references
  class RefCounter {
  protected:
    virtual ~RefCounter() {} // Prevent deallocation by others

  public:
    virtual unsigned getCount() const = 0;
    virtual void incCount() = 0;
    virtual void decCount(const void *ptr) = 0;
    virtual bool isSelfRef() const {return false;}

    static void raise(const std::string &msg);
  };


  class RefCounterPhonyImpl : public RefCounter {
    static RefCounterPhonyImpl singleton;
    RefCounterPhonyImpl() {}

  public:
    static RefCounter *create() {return &singleton;}

    // From RefCounter
    unsigned getCount() const {return 1;}
    void incCount() {};
    void decCount(const void *ptr) {}
  };


  template<typename T, class Dealloc_T = DeallocNew<T> >
  class RefCounterImpl : public RefCounter {
  protected:
    unsigned count;

  public:
    RefCounterImpl(unsigned count = 0) : count(count) {}
    static RefCounter *create() {return new RefCounterImpl;}

    // From RefCounter
    unsigned getCount() const {return count;}
    void incCount() {count++;}

    void decCount(const void *ptr) {
      if (!count) raise("Already zero!");
      if (!--count) release(ptr);
    }

    void release(const void *ptr) {
      if (!isSelfRef()) delete this; // Only deallocate once
      if (ptr) Dealloc_T::dealloc((T *)ptr);
    }
  };


  template<typename T, class Dealloc_T = DeallocNew<T> >
  class ProtectedRefCounterImpl :
    public RefCounterImpl<T, Dealloc_T>, public Mutex {
  protected:
    typedef RefCounterImpl<T, Dealloc_T> Super_T;
    using Super_T::count;

  public:
    ProtectedRefCounterImpl(unsigned count = 0) : Super_T(count) {}
    static RefCounter *create() {return new ProtectedRefCounterImpl;}

    // From RefCounterImpl
    unsigned getCount() const {
      lock();
      unsigned x = count;
      unlock();
      return x;
    }

    void incCount() {lock(); count++; unlock();}

    void decCount(const void *ptr) {
      lock();

      if (!count) {unlock(); Super_T::raise("Already zero!");}

      if (!--count) {
        unlock();
        Super_T::release(ptr);

      } else unlock();
    }
  };


  template<typename T, class Super_T>
  class SelfRefCounterImpl : public Super_T {
  protected:
    bool engaged;

  public:
    SelfRefCounterImpl(unsigned count = 0) : Super_T(count), engaged(false) {}

    bool isSelfRefEngaged() const {return engaged;}
    void selfRef() {if (!engaged) {engaged = true; Super_T::incCount();}}
    void selfDeref() {if (engaged) {engaged = false; Super_T::decCount(this);}}

    // From RefCounter
    bool isSelfRef() const {return true;}
  };
}
