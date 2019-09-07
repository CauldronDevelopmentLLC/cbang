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

#include "EventDB.h"

#include <cbang/event/Base.h>
#include <cbang/event/Event.h>

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Value.h>

#include <mysql/mysqld_error.h>

using namespace std;
using namespace cb;
using namespace cb::MariaDB;


namespace {
  unsigned event_flags_to_db_ready(unsigned flags) {
    unsigned ready = 0;

    if (flags & Event::EventFlag::EVENT_READ)    ready |= DB::READY_READ;
    if (flags & Event::EventFlag::EVENT_WRITE)   ready |= DB::READY_WRITE;
    if (flags & Event::EventFlag::EVENT_CLOSED)  ready |= DB::READY_EXCEPT;
    if (flags & Event::EventFlag::EVENT_TIMEOUT) ready |= DB::READY_TIMEOUT;

    return ready;
  }

  class QueryCallback {
    EventDB &db;
    EventDB::callback_t cb;
    string query;
    unsigned retry;

    typedef enum {
      STATE_START,
      STATE_QUERY,
      STATE_STORE,
      STATE_FETCH,
      STATE_FREE,
      STATE_NEXT,
      STATE_DONE,
    } state_t;

    state_t state;

  public:
    QueryCallback(EventDB &db, EventDB::callback_t cb, const string &query,
                  unsigned retry = 5) :
      db(db), cb(cb), query(query), retry(retry),
      state(STATE_START) {}


    void call(EventDB::state_t state) {
      try {
        cb(state);
        return;
      } CATCH_ERROR;

      if (state != EventDB::EVENTDB_ERROR && state != EventDB::EVENTDB_DONE)
        try {
          cb(EventDB::EVENTDB_ERROR);
        } CATCH_ERROR;
    }


    bool next() {
      switch (state) {
      case STATE_START:
        state = STATE_QUERY;
        LOG_DEBUG(5, "SQL: " << query);
        if (!db.queryNB(query)) return false;

      case STATE_QUERY:
        state = STATE_STORE;
        if (!db.storeResultNB()) return false;

      case STATE_STORE:
        if (!db.haveResult()) {
          if (db.moreResults()) state = STATE_NEXT;
          else state = STATE_DONE;
          return next();
        }

        call(EventDB::EVENTDB_BEGIN_RESULT);
        state = STATE_FETCH;
        if (!db.fetchRowNB()) return false;

      case STATE_FETCH:
        while (db.haveRow()) {
          call(EventDB::EVENTDB_ROW);
          if (!db.fetchRowNB()) return false;
        }
        state = STATE_FREE;
        if (!db.freeResultNB()) return false;

      case STATE_FREE:
        call(EventDB::EVENTDB_END_RESULT);

        if (!db.moreResults()) {
          state = STATE_DONE;
          return next();
        }

      case STATE_NEXT:
        state = STATE_QUERY;
        if (!db.nextResultNB()) return false;
        return next();

      case STATE_DONE:
        LOG_DEBUG(6, "EVENTDB_DONE");
        call(EventDB::EVENTDB_DONE);
        return true;

      default: THROW("Invalid state");
      }
    }


    void operator()(Event::Event &event, int fd, unsigned flags) {
      try {
        if (db.continueNB(event_flags_to_db_ready(flags))) {
          if (!next()) db.renewEvent(event);
        } else db.addEvent(event);

      } catch (const Exception &e) {
        // Retry deadlocks
        if (db.getErrorNumber() == ER_LOCK_DEADLOCK && --retry) {
          LOG_WARNING("DB deadlock detected, retrying");
          call(EventDB::EVENTDB_RETRY);
          state = STATE_START;
          if (!next()) db.renewEvent(event);

        } else {
          LOG_DEBUG(5, e);
          call(EventDB::EVENTDB_ERROR);
        }
      }
    }
  };
}


EventDB::EventDB(Event::Base &base, st_mysql *db) : DB(db), base(base) {}


// TODO Error when EventDB deallocated while events are still outstanding.


unsigned EventDB::getEventFlags() const {
  return
    (waitRead()    ? Event::Base::EVENT_READ    : 0) |
    (waitWrite()   ? Event::Base::EVENT_WRITE   : 0) |
    (waitExcept()  ? Event::Base::EVENT_CLOSED  : 0) |
    (waitTimeout() ? Event::Base::EVENT_TIMEOUT : 0);
}


void EventDB::newEvent(Event::Base::callback_t cb) const {
  assertPending();
  addEvent(*base.newEvent(getSocket(), cb, getEventFlags()));
}


void EventDB::renewEvent(Event::Event &e) const {
  assertPending();
  e.renew(getSocket(), getEventFlags());
  addEvent(e);
}


void EventDB::addEvent(Event::Event &e) const {
  if (waitTimeout()) e.add(getTimeout());
  else e.add();
}


void EventDB::callback(callback_t cb) {
  auto response =
    [this, cb] (Event::Event &e, int fd, unsigned flags) {
      try {
        if (continueNB(event_flags_to_db_ready(flags)))
          TRY_CATCH_ERROR(cb(EventDB::EVENTDB_DONE));
        else addEvent(e);

      } catch (const Exception &e) {
        TRY_CATCH_ERROR(cb(EventDB::EVENTDB_ERROR));
      }
    };

  newEvent(response);
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
  string query = dict.isNull() ? s : format(s, dict->getDict());
  SmartPointer<QueryCallback> queryCB = new QueryCallback(*this, cb, query);

  // By wrapping the event callback in a lambda the SmartPointer is kept alive
  auto reply =
    [queryCB] (Event::Event &event, int fd, unsigned flags) {
      (*queryCB)(event, fd, flags);
    };

  if (isPending() || !queryCB->next()) newEvent(reply);
}
