/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "NameValueTable.h"

#include "Database.h"

#include <cbang/String.h>
#include <cbang/json/Reader.h>

#include <cstdarg>

using namespace std;
using namespace cb;
using namespace cb::DB;


NameValueTable::NameValueTable(Database &db, const string &table) :
  db(db), table(table) {}


void NameValueTable::init() {
  if (foreachStmt.isSet()) return;

  const char *tableName = table.c_str();

  replaceStmt =
    db.compilef("REPLACE INTO \"%s\" (name, value) VALUES (@NAME, @VALUE)",
                tableName);
  deleteStmt = db.compilef("DELETE FROM \"%s\" WHERE name=@NAME", tableName);
  selectStmt =
    db.compilef("SELECT value FROM \"%s\" WHERE name=@NAME", tableName);
  foreachStmt = db.compilef("SELECT name, value FROM \"%s\"", tableName);
}


void NameValueTable::create() {
  db.executef("CREATE TABLE IF NOT EXISTS \"%s\" "
              "(name TEXT PRIMARY KEY, value)", table.c_str());
}


void NameValueTable::setf(const char *name, const char *value, ...) {
  va_list ap;

  va_start(ap, value);
  replaceStmt->parameter(1).bind(String::vprintf(value, ap));
  doSet(name);
  va_end(ap);
}


void NameValueTable::set(const string &name, const string &value) {
  replaceStmt->parameter(1).bind(value);
  doSet(name);
}


void NameValueTable::set(const string &name, int64_t value) {
  replaceStmt->parameter(1).bind(value);
  doSet(name);
}


void NameValueTable::set(const string &name, double value) {
  replaceStmt->parameter(1).bind(value);
  doSet(name);
}


void NameValueTable::set(const string &name, bool value) {
  replaceStmt->parameter(1).bind(value);
  doSet(name);
}


void NameValueTable::set(const string &name) {
  replaceStmt->parameter(1).bind();
  doSet(name);
}


void NameValueTable::unset(const string &name) {
  deleteStmt->parameter(0).bind(name);
  deleteStmt->execute();
  deleteStmt->clearBindings();
}


const string NameValueTable::getString(const string &name) const {
  string x = doGet(name).toString();
  selectStmt->reset();
  return x;
}


int64_t NameValueTable::getInteger(const string &name) const {
  int64_t x = doGet(name).toInteger();
  selectStmt->reset();
  return x;
}


double NameValueTable::getDouble(const string &name) const {
  double x = doGet(name).toDouble();
  selectStmt->reset();
  return x;
}


bool NameValueTable::getBoolean(const string &name) const {
  bool x = doGet(name).toBoolean();
  selectStmt->reset();
  return x;
}


SmartPointer<JSON::Value> NameValueTable::getJSON(const string &name) const {
  return JSON::Reader::parse(getString(name));
}


const string NameValueTable::getString(const string &name,
                                       const string &defaultValue) const {
  return has(name) ? getString(name) : defaultValue;
}


int64_t NameValueTable::getInteger(const string &name,
                                   int64_t defaultValue) const {
  return has(name) ? getInteger(name) : defaultValue;
}


double NameValueTable::getDouble(const string &name,
                                 double defaultValue) const {
  return has(name) ? getDouble(name) : defaultValue;
}


bool NameValueTable::getBoolean(const string &name, bool defaultValue) const {
  return has(name) ? getBoolean(name) : defaultValue;
}


SmartPointer<JSON::Value>
NameValueTable::getJSON(const string &name,
                        const SmartPointer<JSON::Value> &defaultValue) const {
  return has(name) ? getJSON(name) : defaultValue;
}


bool NameValueTable::has(const string &name) const {
  selectStmt->parameter(0).bind(name);
  bool hasValue = selectStmt->next();
  selectStmt->reset();
  return hasValue;
}


void NameValueTable::foreach(function<void (const string &,
                                            const string &)> cb) {
  if (!cb) THROW("Callback cannot be null");

  while (foreachStmt->next())
    cb(foreachStmt->column(0).toString(), foreachStmt->column(1).toString());

  foreachStmt->reset();
}


Column NameValueTable::doGet(const string &name) const {
  selectStmt->parameter(0).bind(name);

  if (!selectStmt->next()) {
    selectStmt->reset();
    THROW("'" << name << "' not found in NameValueTable '" << table << "'");
  }

  return selectStmt->column(0);
}


void NameValueTable::doSet(const std::string &name) {
  replaceStmt->parameter(0).bind(name);
  replaceStmt->execute();
  replaceStmt->clearBindings();
}
