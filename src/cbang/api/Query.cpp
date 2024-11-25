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

#include "Query.h"
#include "API.h"
#include "Resolver.h"

#include <cbang/log/Logger.h>

#include <mysql/mysqld_error.h>

using namespace std;
using namespace cb;
using namespace cb::API;


Query::Query(
  API &api, const SmartPointer<HTTP::Request> &req, const string &sql,
  const string &returnType, const JSON::ValuePtr &fields) :
  api(api), req(req), sql(sql), returnType(returnType), fields(fields) {}


void Query::query(callback_t cb) {
  if (!cb) THROW("Callback not set");
  this->cb = cb;

  if (db.isNull()) db = api.getDBConnector().getConnection();
  string sql = Resolver(api, req).format(this->sql, "NULL");

  // Stay alive until query callbacks are complete
  auto self = SmartPtr(this);
  auto queryCB = [self] (state_t state) {self->callback(state);};

  db->query(queryCB, sql);
}


void Query::callback(state_t state) {
  if (returnType == "ok")     return Query::returnOk    (state);
  if (returnType == "hlist")  return Query::returnHList (state);
  if (returnType == "list")   return Query::returnList  (state);
  if (returnType == "fields") return Query::returnFields(state);
  if (returnType == "dict")   return Query::returnDict  (state);
  if (returnType == "one")    return Query::returnOne   (state);
  if (returnType == "bool")   return Query::returnBool  (state);
  if (returnType == "u64")    return Query::returnU64   (state);
  if (returnType == "s64")    return Query::returnS64   (state);
  if (returnType == "pass")   return Query::returnOk    (state);
  THROW("Unsupported query return type '" << returnType << "'");
}


void Query::reply(HTTP::Status code) {
  writer.close();
  cb(code, writer);
}


void Query::errorReply(HTTP::Status code, const string &msg) {
  writer.reset();

  writer.beginDict();
  writer.insert("error", msg.empty() ? code.toString() : msg);
  writer.endDict();
  writer.close();

  reply(code);
}


void Query::returnHList(MariaDB::EventDB::state_t state) {
  if (state == MariaDB::EventDB::EVENTDB_ROW) {
    if (!rowCount++) {
      writer.beginList();

      // Write list header
      writer.appendList();
      for (unsigned i = 0; i < db->getFieldCount(); i++)
        writer.append(db->getField(i).getName());
      writer.endList();
    }

    writer.beginAppend();
    db->writeRowList(writer);

  } else returnList(state);
}



void Query::returnList(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (!rowCount++) writer.beginList();

    writer.beginAppend();

    if (db->getFieldCount() == 1) db->writeField(writer, 0);
    else db->writeRowDict(writer);
    break;

  case MariaDB::EventDB::EVENTDB_DONE:
    if (!rowCount) writer.beginList(); // Empty list
    writer.endList();
    returnOk(state);
    break;

  default: returnOk(state); break;
  }
}


void Query::returnBool(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) writer.writeBoolean(db->getBoolean(0));
}



void Query::returnU64(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) writer.write(db->getU64(0));
}


void Query::returnS64(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) writer.write(db->getS64(0));
}


void Query::returnFields(MariaDB::EventDB::state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_ROW:
    if (!rowCount++) writer.beginDict();

    if (!nextField.empty()) {
      closeField = true;
      if (nextField[0] == '*') {
        if (nextField.length() != 1) writer.insertDict(nextField.substr(1));
        else closeField = false;
      } else writer.insertList(nextField);
      nextField.clear();
    }

    if (writer.inDict()) db->insertRow(writer, 0, -1, false);

    else if (db->getFieldCount() == 1) {
      writer.beginAppend();
      db->writeField(writer, 0);

    } else {
      writer.appendDict();
      db->insertRow(writer, 0, -1, false);
      writer.endDict();
    }
    break;

  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT: {
    closeField = false;
    if (fields.isNull()) THROW("Fields cannot be null");
    if (currentField == fields->size()) THROW("Unexpected DB result");
    nextField = fields->getString(currentField++);
    if (nextField.empty()) THROW("Empty field name");
    break;
  }

  case MariaDB::EventDB::EVENTDB_END_RESULT:
    if (closeField) {
      if (writer.inList()) writer.endList();
      else writer.endDict();
    }
    break;

  case MariaDB::EventDB::EVENTDB_DONE:
    if (!rowCount) errorReply(HTTP_NOT_FOUND);
    else {
      writer.endDict();
      return returnOk(state); // Success
    }
    break;

  default: return returnOk(state);
  }
}


void Query::returnDict(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) db->writeRowDict(writer);
}


void Query::returnOne(MariaDB::EventDB::state_t state) {
  if (checkOne(state)) db->writeField(writer, 0);
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
  case MariaDB::EventDB::EVENTDB_BEGIN_RESULT: break;
  case MariaDB::EventDB::EVENTDB_END_RESULT: break;
  case MariaDB::EventDB::EVENTDB_DONE: reply(); break;
  case MariaDB::EventDB::EVENTDB_RETRY: writer.reset(); break;

  case MariaDB::EventDB::EVENTDB_ERROR: {
    HTTP::Status error = HTTP_INTERNAL_SERVER_ERROR;

    switch (db->getErrorNumber()) {
    case ER_SIGNAL_NOT_FOUND: case ER_FILE_NOT_FOUND:
      error = HTTP_NOT_FOUND;
      break;

    case ER_DUP_ENTRY:           error = HTTP_CONFLICT;    break;
    case ER_SIGNAL_EXCEPTION:    error = HTTP_BAD_REQUEST; break;

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
