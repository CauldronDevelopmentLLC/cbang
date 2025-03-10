/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Event.h"

#include <cbang/Catch.h>
#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>
#include <cbang/debug/Debugger.h>

#include <event2/event.h>
#include <event2/event_struct.h>

#include <limits>
#include <cstdlib>
#include <cmath>

using namespace cb::Event;
using namespace std;


namespace {
  void event_cb(evutil_socket_t fd, short flags, void *arg) {
    ((Event *)arg)->call(fd, flags);
  }
}


Event::Event(Base &base, cb::socket_t fdOrSignal, callback_t cb,
             unsigned flags) :
  e(event_new(base.getBase(), fdOrSignal, flags & 0xff, event_cb, this)),
  cb(cb) {if (!e) THROW("Failed to create event");}


Event::~Event() {
  LOG_DEBUG(6, CBANG_FUNC << "() " << Debugger::getStackTrace());
  if (e) event_free(e);
}


double Event::getTimeout() const {
  struct timeval tv;

  if (event_pending(e, EV_TIMEOUT, &tv))
    return Timer::toDouble(tv) - Timer::now();

  return numeric_limits<double>::infinity();
}


bool Event::isPending(unsigned events) const {
  events &= EV_TIMEOUT | EV_READ | EV_WRITE | EV_SIGNAL;
  return event_pending(e, events, 0);
}


unsigned Event::getEvents() const {return event_get_events(e);}
int Event::getFD() const {return event_get_fd(e);}


void Event::setPriority(int priority) {
  if (event_priority_set(e, priority)) THROW("Failed to set event priority");
}


int Event::getPriority() const {return event_get_priority(e);}


void Event::assign(Base &base, int fd, unsigned events, callback_t cb) {
  del();
  this->cb = cb;
  event_assign(e, base.getBase(), fd, events, event_cb, this);
}


void Event::renew(int fd, unsigned events) {
  del();
  event_assign(e, event_get_base(e), fd, events, event_cb, this);
}


void Event::renew(int signal) {
  del();
  event_assign(e, event_get_base(e), signal, EV_SIGNAL, event_cb, this);
}


void Event::add(double t) {
  struct timeval tv = Timer::toTimeVal(max(0.0, t));
  event_add(e, &tv);
}


void Event::add() {event_add(e, 0);}


void Event::readd() {
  struct timeval tv = e->ev_timeout;
  event_add(e, &tv);
}


void Event::next(uint64_t period) {
  auto now = Timer::now();
  add((1 + floor(now / period)) * period - now + 0.1);
}


void Event::del() {event_del(e);}
void Event::activate(int flags) {event_active(e, flags, 0);}


void Event::call(int fd, short flags) {
  LOG_DEBUG(0 <= fd ? 5 : 6, "Event callback fd=" << fd << " flags=" << flags);
  auto self = SmartPtr(this); // Don't deallocate while in callback
  TRY_CATCH_ERROR(cb(*this, fd, flags));
}


string Event::getEventsString(unsigned events) {
  vector<string> parts;

  if (events & EVENT_TIMEOUT)   parts.push_back("TIMEOUT");
  if (events & EVENT_READ)      parts.push_back("READ");
  if (events & EVENT_WRITE)     parts.push_back("WRITE");
  if (events & EVENT_SIGNAL)    parts.push_back("SIGNAL");
  if (events & EVENT_PERSIST)   parts.push_back("PERSIST");
  if (events & EVENT_EDGE_TRIG) parts.push_back("EDGE_TRIG");

  return String::join(parts, "|");
}


void Event::enableDebugMode() {event_enable_debug_mode();}


namespace {
  static int event_log_level = 0;

  void log_cb(int severity, const char *msg) {
    switch (severity) {
    case _EVENT_LOG_DEBUG:
      LOG_DEBUG(event_log_level, "Event: " << msg);
      break;

    case _EVENT_LOG_MSG:
      LOG_DEBUG(event_log_level, "Event: " << msg);
      break;

    case _EVENT_LOG_WARN:
      LOG_WARNING("Event: " << msg);
      break;

    case _EVENT_LOG_ERR:
      LOG_ERROR("Event: " << msg);
      break;
    }
  }


  void fatal_cb(int err) {
    LOG_ERROR("Fatal error in event system " << err << ": "
              << cb::Debugger::getStackTrace());
    abort();
  }
}


void Event::enableLogging(int level) {
  event_log_level = level;
  event_set_log_callback(log_cb);
  event_set_fatal_callback(fatal_cb);
}


void Event::enableDebugLogging() {event_enable_debug_logging(EVENT_DBG_ALL);}
