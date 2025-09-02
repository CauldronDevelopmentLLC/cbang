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

#include "QueryCallback.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>

#include <mysql/mysqld_error.h>

using namespace std;
using namespace cb;
using namespace cb::MariaDB;


QueryCallback::QueryCallback(EventDB &db, EventDB::callback_t cb,
                             const string &query, unsigned retry) :
  db(db), cb(cb), query(query), retry(retry) {}


void QueryCallback::call(EventDB::state_t state) {
  try {
    cb(state);
    return;
  } CATCH_ERROR;

  if (state != EventDB::EVENTDB_ERROR && state != EventDB::EVENTDB_DONE)
    try {
      cb(EventDB::EVENTDB_ERROR);
    } CATCH_ERROR;
}


bool QueryCallback::next() {
  LOG_DEBUG(6, CBANG_FUNC << "() state=" << state);

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
    // Limit number of rows processed per event
    unsigned i;
    for (i = 0; i < 1000 && db.haveRow(); i++) {
      call(EventDB::EVENTDB_ROW);
      if (!db.fetchRowNB()) return false;
    }

    if (db.haveRow()) return resume = true;
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


void QueryCallback::operator()(Event::Event &, int, unsigned flags) {
  LOG_DEBUG(6, CBANG_FUNC << "() flags=" << flags);

  try {
    if (resume || db.continueNB(db.eventFlagsToDBReady(flags))) {
      resume = false;
      if (!next()) return db.renewEvent();
      if (resume)  return db.rescheduleEvent();
    } else return db.addEvent();

  } catch (const Exception &e) {
    // Retry deadlocks
    if (db.getErrorNumber() == ER_LOCK_DEADLOCK && --retry) {
      LOG_WARNING("DB deadlock detected, retrying");
      call(EventDB::EVENTDB_RETRY);
      state = STATE_START;
      if (!next()) return db.renewEvent();

    } else {
      LOG_DEBUG(5, e);
      call(EventDB::EVENTDB_ERROR);
    }
  }

  db.endEvent();
}
