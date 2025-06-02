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

#include "Query.h"
#include "QueryDef.h"

#include <cbang/log/Logger.h>

#include <mysql/mysqld_error.h>

using namespace std;
using namespace cb;
using namespace cb::API;


Query::Query(const SmartPointer<const QueryDef> &def, callback_t cb) :
  def(def), cb(cb), db(def->getDBConnection()) {
  if (!cb) THROW("Callback not set");
}


void Query::exec(const string &sql) {
  // Stay alive until query callbacks are complete
  auto self = SmartPtr(this);
  auto cb = [self] (state_t state) {self->callback(state);};
  db->query(cb, sql);
}


Query::return_t Query::getReturnType(const string &name) {
  if (name == "ok")     return &Query::returnOk;
  if (name == "hlist")  return &Query::returnHList;
  if (name == "list")   return &Query::returnList;
  if (name == "fields") return &Query::returnFields;
  if (name == "dict")   return &Query::returnDict;
  if (name == "one")    return &Query::returnOne;
  if (name == "bool")   return &Query::returnBool;
  if (name == "u64")    return &Query::returnU64;
  if (name == "s64")    return &Query::returnS64;

  THROW("Unsupported query return type '" << name << "'");
}


void Query::callback(state_t state) {(*this.*def->returnCB)(state);}


void Query::reply(HTTP::Status code) {
  sink.close();
  cb(code, sink.getRoot());
}


void Query::errorReply(HTTP::Status code, const string &msg) {
  sink.reset();

  sink.beginDict();
  sink.insert("error", msg.empty() ? code.toString() : msg);
  sink.endDict();
  sink.close();

  reply(code);
}


void Query::returnHList(MariaDB::EventDB::state_t state) {
  if (state == MariaDB::EventDB::EVENTDB_ROW) {
    if (!rowCount++) {
      sink.beginList();

      // Write list header
      sink.appendList();
      for (unsigned i = 0; i < db->getFieldCount(); i++)
        sink.append(db->getField(i).getName());
      sink.endList();
    }

    sink.beginAppend();
    db->writeRowList(sink);

  } else returnList(state);
}



void Query::returnList(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (!rowCount++) sink.beginList();

    sink.beginAppend();

    if (db->getFieldCount() == 1) db->writeField(sink, 0);
    else db->writeRowDict(sink);
    break;

  case MariaDB::EventDB::EVENTDB_DONE:
    if (!rowCount) sink.beginList(); // Empty list
    sink.endList();
    returnOk(state);
    break;

  default: returnOk(state); break;
  }
}


void Query::returnBool(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) sink.writeBoolean(db->getBoolean(0));
}


void Query::returnU64(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) sink.write(db->getU64(0));
}


void Query::returnS64(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) sink.write(db->getS64(0));
}


void Query::returnFields(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (!rowCount++) sink.beginDict();

    if (!nextField.empty()) {
      closeField = true;
      if (nextField[0] == '*') {
        if (nextField.length() != 1) sink.insertDict(nextField.substr(1));
        else closeField = false;
      } else sink.insertList(nextField);
      nextField.clear();
    }

    if (sink.inDict()) db->insertRow(sink, 0, -1, false);

    else if (db->getFieldCount() == 1) {
      sink.beginAppend();
      db->writeField(sink, 0);

    } else {
      sink.appendDict();
      db->insertRow(sink, 0, -1, false);
      sink.endDict();
    }
    break;

  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT: {
    closeField = false;
    if (def->fields.isNull()) THROW("Fields cannot be null");
    if (currentField == def->fields->size()) THROW("Unexpected DB result");
    nextField = def->fields->getString(currentField++);
    if (nextField.empty()) THROW("Empty field name");
    break;
  }

  case MariaDB::EventDB::EVENTDB_END_RESULT:
    if (closeField) sink.end();
    break;

  case MariaDB::EventDB::EVENTDB_DONE:
    if (!rowCount) errorReply(HTTP_NOT_FOUND);
    else {
      sink.endDict();
      return returnOk(state); // Success
    }
    break;

  default: return returnOk(state);
  }
}


void Query::returnDict(MariaDB::EventDB::state_t state) {
  if (state == MariaDB::EventDB::EVENTDB_ROW) {
    if (!rowCount++) db->writeRowDict(sink);
    else returnOk(state); // Error

  } else checkOne(state);
}


void Query::returnOne(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) db->writeField(sink, 0);
}


bool Query::checkOne(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (!rowCount++ && db->getFieldCount() == 1) return true;
    returnOk(state); // Error
    break;

  case MariaDB::EventDB::EVENTDB_DONE:
    if (!rowCount) errorReply(HTTP_NOT_FOUND);
    else returnOk(state); // Success
    break;

  default: returnOk(state);
  }

  return false;
}


void Query::returnOk(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT:               break;
  case MariaDB::EventDB::EVENTDB_END_RESULT:                 break;
  case MariaDB::EventDB::EVENTDB_DONE:         reply();      break;
  case MariaDB::EventDB::EVENTDB_RETRY:        sink.reset(); break;

  case MariaDB::EventDB::EVENTDB_ERROR: {
    HTTP::Status error = HTTP_INTERNAL_SERVER_ERROR;

    switch (db->getErrorNumber()) {
    case ER_SIGNAL_NOT_FOUND: case ER_FILE_NOT_FOUND:
      error = HTTP_NOT_FOUND;
      break;

    case ER_DUP_ENTRY:        error = HTTP_CONFLICT;    break;
    case ER_SIGNAL_EXCEPTION: error = HTTP_BAD_REQUEST; break;

    case ER_ACCESS_DENIED_ERROR: case ER_DBACCESS_DENIED_ERROR:
      error = HTTP_UNAUTHORIZED;
      break;

    default: break;
    }

    string errMsg =
      SSTR("DB:" << db->getErrorNumber() << ": " << db->getError());
    errorReply(error, errMsg);
    if (error == HTTP_INTERNAL_SERVER_ERROR) THROW(errMsg);
    LOG_WARNING(errMsg);

    break;
  }

  case MariaDB::EventDB::EVENTDB_ROW:
    LOG_ERROR("Unexpected DB row");
    errorReply(HTTP_INTERNAL_SERVER_ERROR, "Unexpected DB row");
    break;

  default:
    errorReply(HTTP_INTERNAL_SERVER_ERROR, "Unexpected DB response");
    THROW("Unexpected DB response: " << state);
  }
}
