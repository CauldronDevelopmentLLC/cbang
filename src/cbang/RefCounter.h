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

#include "Deallocators.h"

#include <atomic>
#include <string>
#include <cstdint>


namespace cb {
  class RefCounted;


  /// This class is used by SmartPointer to count pointer references
  class RefCounter {
  protected:
    virtual ~RefCounter() {} // Prevent deallocation by others

  public:
    static RefCounter *_getCounter(const void *ptr) {return 0;}
    static RefCounter *_getCounter(const RefCounted *ptr);
    static void _setCounter(const void *ptr, RefCounter *counter) {}
    static void _setCounter(const RefCounted *ptr, RefCounter *counter);

    virtual bool isActive() const = 0;
    virtual unsigned getCount(bool weak) const = 0;
    virtual void incCount(bool weak) = 0;
    virtual bool tryIncStrong() = 0;
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
    std::atomic<uint64_t> counts{0x100000000ULL}; // Start with one strong

    RefCounterImpl(T *ptr) : ptr(ptr) {}

  public:
    static unsigned trace;

    static RefCounter *getCounter(T *ptr, bool weak) {
      RefCounter *counter = RefCounter::_getCounter(ptr);

      if (counter) {
        // Existing counter — increment appropriately
        if (weak) counter->incCount(true);
        else if (!counter->tryIncStrong()) return 0; // Object destroyed
        return counter;
      }

      if (weak) raise("Can't create weak pointer to unmanaged object!");

      // New counter, born with strong count of 1
      counter = new RefCounterImpl<T, DeallocT>(ptr);
      _setCounter(ptr, counter);
      return counter;
    }

    // From RefCounter
    bool isActive() const override {return getCount(false);}

    unsigned getCount(bool weak) const override {
      return weak ? (counts & 0xffffffff) : (counts >> 32);
    }

    void incCount(bool weak) override {
      uint64_t delta = weak ? 1 : 0x100000000ULL;
      uint64_t prev  = counts.fetch_add(delta, std::memory_order_acq_rel);
      uint32_t c     = (weak ? prev & 0xffffffff : prev >> 32) + 1;
      log(trace, "incCount() count=%u", c);
    }

    bool tryIncStrong() override {
      uint64_t c = counts.load(std::memory_order_relaxed);
      do {
        if (!(c >> 32)) return false; // Object already destroyed
      } while (!counts.compare_exchange_weak(c, c + 0x100000000ULL,
        std::memory_order_acq_rel, std::memory_order_relaxed));
      return true;
    }

    void decCount(bool weak) override {
      // Decrement the appropriate half atomically
      uint64_t delta = weak ? 1 : 0x100000000ULL;
      uint64_t prev = counts.fetch_sub(delta, std::memory_order_acq_rel);

      uint32_t strongPrev = prev >> 32;
      uint32_t weakPrev   = prev & 0xffffffff;

      uint32_t c = weak ? weakPrev : strongPrev;
      if (!c) raise("Already zero!");

      log(trace, "decCount() count=%u", c - 1);

      if (!weak && strongPrev == 1) {
        // Strong count just hit zero — deallocate the object
        T *_ptr = ptr;
        ptr = 0;
        if (_ptr) DeallocT::dealloc(_ptr);

        // If there are also no weak refs, destroy the counter itself
        if (!weakPrev) delete this;
      }

      // Delete if weak count just hit zero and strong was already gone
      if (weak && weakPrev == 1 && !strongPrev) delete this;
    }

    void adopted() override {
      uint64_t c      = counts.load(std::memory_order_relaxed);
      uint32_t strong = c >> 32;
      uint32_t weak   = c & 0xffffffff;

      if (1 < strong + weak)
        raise("Can't adopt pointer with multiple references!");
      _setCounter(ptr, 0);
      delete this;
    }
  };


  template<typename T, class DeallocT>
  unsigned RefCounterImpl<T, DeallocT>::trace = 0;


  class RefCounterPhonyImpl : public RefCounter {
    static RefCounterPhonyImpl singleton;
    RefCounterPhonyImpl() {}

  public:
    static RefCounter *getCounter(const void *, bool) {return &singleton;}

    // From RefCounter
    bool isActive() const override {return true;}
    unsigned getCount(bool weak) const override {return 1;}
    void incCount(bool weak) override {}
    bool tryIncStrong() override {return true;}
    void decCount(bool weak) override {}
    void adopted() override {}
  };
}
