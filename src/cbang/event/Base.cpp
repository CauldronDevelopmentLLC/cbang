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

#include "Base.h"
#include "Event.h"
#include "FDPool.h"

#include <event2/thread.h>
#include <event2/event.h>
#include <event2/event_struct.h>

#include <cbang/Exception.h>
#include <cbang/net/Socket.h>
#include <cbang/os/SysError.h>
#include <cbang/dns/Base.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


namespace {
  extern "C" int count_events_by_priority(
    const struct event_base *base, const struct event *event, void *arg) {
    if (!(event->ev_evcallback.evcb_flags &
          (EVLIST_ACTIVE | EVLIST_ACTIVE_LATER))) return 0;

    map<int, unsigned> &counts = *(map<int, unsigned> *)arg;
    int priority = event_get_priority(event);
    counts.insert(pair<int, unsigned>(priority, 0)).first->second++;
    return 0;
  }
}

bool Base::_threadsEnabled = false;


Base::Base(bool withThreads, int priorities) : EventFactory(*this) {
  Socket::initialize(); // Windows needs this

  if (withThreads) enableThreads();
  base = event_base_new();
  if (!base) THROW("Failed to create event base");
  if (0 < priorities) initPriority(priorities);
}


Base::~Base() {
  deallocating = true;

  // Must be released before base is freed
  dns.release();
  pool.release();

  if (base) event_base_free(base);
}


DNS::Base &Base::getDNS() {
  if (deallocating) THROW("Base deallocating");
  if (dns.isNull()) dns = new DNS::Base(*this);
  return *dns;
}


FDPool &Base::getPool() {
  if (deallocating) THROW("Base deallocating");
  if (pool.isNull()) pool = FDPool::create(*this);
  return *pool;
}


void Base::initPriority(int num) {
  if (event_base_priority_init(base, num))
    THROW("Failed to init event base priority");
}


int Base::getNumPriorities() const {
  return event_base_get_npriorities(base);
}


int Base::getNumEvents() const {
  return event_base_get_num_events(base, EVENT_BASE_COUNT_ADDED);
}


int Base::getNumActiveEvents() const {
  return event_base_get_num_events(base, EVENT_BASE_COUNT_ACTIVE);
}


void Base::countActiveEventsByPriority(map<int, unsigned> &counts) const {
  event_base_foreach_event(base, count_events_by_priority, &counts);
}


void Base::dispatch() {
  if (event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY))
    THROW("Event dispatch failed: " << SysError());
}


void Base::loop() {if (event_base_loop(base, 0)) THROW("Loop failed");}


void Base::loopOnce() {
  if (event_base_loop(base, EVLOOP_ONCE)) THROW("Loop once failed");
}


bool Base::loopNonBlock() {
  int ret = event_base_loop(base, EVLOOP_NONBLOCK);
  if (ret == -1) THROW("Loop nonblock failed");
  return ret == 0;
}


void Base::loopBreak() {
  if (event_base_loopbreak(base)) THROW("Loop break failed");
}


void Base::loopContinue() {
  if (event_base_loopcontinue(base)) THROW("Loop continue failed");
}


void Base::loopExit() {
  if (event_base_loopexit(base, 0)) THROW("Loop exit failed");
}


void Base::enableThreads() {
  if (_threadsEnabled) return;

#if EVTHREAD_USE_PTHREADS_IMPLEMENTED
  if (evthread_use_pthreads())
    THROW("Failed to enable libevent thread support");

#elif EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
  if (evthread_use_windows_threads())
    THROW("Failed to enable libevent thread support");

#else
  THROW("libevent not built with thread support");
#endif

  _threadsEnabled = true;
}
