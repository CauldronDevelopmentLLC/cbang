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

#include "EventDB.h"
#include "QueryCallback.h"

#include <cbang/event/Base.h>
#include <cbang/event/Event.h>

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Value.h>

#include <mysql/mysqld_error.h>

using namespace std;
using namespace cb;
using namespace cb::MariaDB;
using namespace cb::Event;


EventDB::EventDB(Base &base, st_mysql *db) : DB(db), base(base) {}
EventDB::~EventDB() {LOG_DEBUG(5, CBANG_FUNC << "()");}


unsigned EventDB::getEventFlags() const {
  return
    (waitRead()    ? Base::EVENT_READ    : 0) |
    (waitWrite()   ? Base::EVENT_WRITE   : 0) |
    (waitExcept()  ? Base::EVENT_CLOSED  : 0) |
    (waitTimeout() ? Base::EVENT_TIMEOUT : 0);
}


void EventDB::connect(callback_t cb, const string &host, const string &user,
                      const string &password, const string &dbName,
                      unsigned port, const string &socketName, flags_t flags) {
  try {
    if (connectNB(host, user, password, dbName, port, socketName, flags))
      TRY_CATCH_ERROR(cb(EVENTDB_DONE));
    else callback(cb);

  } catch (const Exception &e) {
    TRY_CATCH_ERROR(cb(EventDB::EVENTDB_ERROR));
  }
}


void EventDB::ping(callback_t cb) {
  try {
    if (pingNB()) TRY_CATCH_ERROR(cb(EVENTDB_DONE));
    else callback(cb);

  } catch (const Exception &e) {
    TRY_CATCH_ERROR(cb(EventDB::EVENTDB_ERROR));
  }
}


void EventDB::close(callback_t cb) {
  try {
    if (closeNB()) TRY_CATCH_ERROR(cb(EVENTDB_DONE));
    else callback(cb);

  } catch (const Exception &e) {
    TRY_CATCH_ERROR(cb(EventDB::EVENTDB_ERROR));
  }
}


void EventDB::query(callback_t cb, const string &s,
                    const SmartPointer<const JSON::Value> &dict) {
  LOG_DEBUG(5, CBANG_FUNC << "() sql=" << s);

  string query = dict.isNull() ? s : format(s, dict->getDict());
  auto queryCB = SmartPtr(new QueryCallback(*this, cb, query));

  // By wrapping the event callback in a lambda the SmartPointer is kept alive
  auto reply =
    [queryCB] (Event::Event &event, int fd, unsigned flags) {
      (*queryCB)(event, fd, flags);
    };

  if (isPending() || !queryCB->next()) newEvent(reply);
}


unsigned EventDB::eventFlagsToDBReady(unsigned flags) {
  unsigned ready = 0;

  if (flags & EventFlag::EVENT_READ)    ready |= DB::READY_READ;
  if (flags & EventFlag::EVENT_WRITE)   ready |= DB::READY_WRITE;
  if (flags & EventFlag::EVENT_CLOSED)  ready |= DB::READY_EXCEPT;
  if (flags & EventFlag::EVENT_TIMEOUT) ready |= DB::READY_TIMEOUT;

  return ready;
}


void EventDB::newEvent(Base::callback_t cb) {
  LOG_DEBUG(5, CBANG_FUNC << "() socket=" << getSocket()
    << " flags=" << getEventFlags());
  if (event.isSet() && event->isPending()) THROW("Event already pending");
  assertPending();
  event = base.newEvent(getSocket(), cb, getEventFlags());
  event->add();
}


void EventDB::renewEvent() const {
  assertPending();
  event->renew(getSocket(), getEventFlags());
  addEvent();
}


void EventDB::addEvent() const {
  if (waitTimeout()) event->add(getTimeout());
  else event->add();
}


void EventDB::callback(callback_t cb) {
  auto response =
    [this, cb] (Event::Event &e, int fd, unsigned flags) {
      try {
        if (continueNB(eventFlagsToDBReady(flags)))
          TRY_CATCH_ERROR(cb(EventDB::EVENTDB_DONE));
        else addEvent();

      } catch (const Exception &e) {
        TRY_CATCH_ERROR(cb(EventDB::EVENTDB_ERROR));
      }
    };

  newEvent(response);
}
