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

#include "Base.h"
#include "Event.h"

#include <event2/thread.h>
#include <event2/event.h>

#include <cbang/Exception.h>
#include <cbang/socket/Socket.h>

using namespace cb::Event;
using namespace cb;


bool Base::_threadsEnabled = false;


Base::Base(bool withThreads) {
  Socket::initialize(); // Windows needs this

  if (withThreads) enableThreads();
  base = event_base_new();
  if (!base) THROW("Failed to create event base");
}


Base::~Base() {if (base) event_base_free(base);}


void Base::initPriority(int num) {
  if (event_base_priority_init(base, num))
    THROW("Failed to init event base priority");
}


int Base::getNumPriorities() const {
  return event_base_get_npriorities(base);
}


SmartPointer<cb::Event::Event>
Base::newEvent(callback_t cb, unsigned flags) {return newEvent(-1, cb, flags);}


SmartPointer<cb::Event::Event>
Base::newEvent(socket_t fd, callback_t cb, unsigned flags) {
  return new Event(*this, fd, cb, flags);
}


SmartPointer<cb::Event::Event>
Base::newSignal(int signal, callback_t cb, unsigned flags) {
  return newEvent((socket_t)signal, cb, flags | EV_SIGNAL);
}


void Base::dispatch() {if (event_base_dispatch(base)) THROW("Dispatch failed");}
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
  if (event_base_loopcontinue(base)) THROW("Loop break failed");
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
