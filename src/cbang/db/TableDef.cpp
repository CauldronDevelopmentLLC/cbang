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

#include "TableDef.h"

#include "Database.h"

#include <cbang/String.h>
#include <cbang/Exception.h>

#include <cbang/Catch.h>

#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::DB;


string TableDef::getEscapedName(const string &name) {
  vector<string> parts;
  String::tokenize(name, parts, ".");
  return "\"" + String::join(parts, "\".\"") + "\"";
}


void TableDef::add(const ColumnDef &column) {
  columnMap[column.getName()] = columns.size();
  columns.push_back(column);
}


unsigned TableDef::getIndex(const std::string &column) const {
  columnMap_t::const_iterator it = columnMap.find(column);
  if (it == columnMap.end())
    THROW("Index for column '" << column << "' not found");
  return it->second;
}


void TableDef::create(Database &db) const {
  ostringstream sql;

  sql << "CREATE TABLE IF NOT EXISTS " << getEscapedName() << " (";

  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql << ",";
    sql << '"' << it->getName() << "\" " << it->getType() << " "
        << it->getConstraints();
  }

  if (constraints != "") {
    if (!columns.empty()) sql << ",";
    sql << constraints;
  }

  sql << ")";

  db.execute(sql.str());
}


void TableDef::rebuild(Database &db, const columnRemap_t &_columnRemap) const {
  string escName = getEscapedName();

  // Parse existing structure
  string result;
  if (!db.execute(string("SELECT sql FROM sqlite_master WHERE name=") +
                  escName + " AND type=\"table\"", result))
    THROW("Failed to read " << name << " table structure");

  {
    size_t start = result.find('(');
    size_t end = result.find_last_of(')');
    if (start == string::npos || end == string::npos)
      THROW("Failed to parse " << name << " table structure: " << result);

    result = result.substr(start + 1, end - start - 1);
  }

  vector<string> cols;
  String::tokenize(result, cols, ",");

  // Remap columns
  CIStringMap columnRemap(_columnRemap);
  for (unsigned i = 0; i < cols.size() - 1; i++) {
    size_t length = cols[i].find(' ');
    if (length == string::npos)
      THROW("Failed to parse " << name << " table structure at: " << cols[i]);

    string col = String::trim(cols[i].substr(0, length), "\"");

    if (columnRemap.find(col) == columnRemap.end())
      columnRemap[col] = col;
  }

  // Create SQL copy command
  string sql = "INSERT INTO " + escName + " SELECT ";

  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql += ", ";

    columnRemap_t::iterator it2 = columnRemap.find(it->getName());
    if (it2 == columnRemap.end()) {
      string type = String::toLower(it->getType());

      if (type == "text") sql += "\"\"";
      else if (type == "blob") sql += "NULL";
      else sql += "0";

    } else sql += string("\"") + it2->second + "\"";
  }

  string tmpName = getEscapedName(name + "_Tmp");
  sql += " FROM " + tmpName;

  // Move table to tmp
  db.execute("ALTER TABLE " + escName + " RENAME TO " + tmpName);

  // Recreate table using new definition
  create(db);

  // Copy data
  try {
    db.execute(sql);
  } CBANG_CATCH_ERROR;

  // Delete temp
  db.execute("DROP TABLE " + tmpName);
}


void TableDef::deleteAll(Database &db) const {
  db.execute("DELETE FROM " + getEscapedName());
}


SmartPointer<Statement>
TableDef::makeWriteStmt(Database &db, const string &suffix) const {
  ostringstream sql;

  sql << "REPLACE INTO " << getEscapedName() << " (";

  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql << ", ";
    sql << '"' << it->getName() << '"';
  }

  sql << ") VALUES (";

  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql << ", ";

    sql << "@" << String::toUpper(it->getName());
  }

  sql << ") " << suffix;

  return db.compile(sql.str());
}


SmartPointer<Statement>
TableDef::makeReadStmt(Database &db, const string &suffix) const {
  ostringstream sql;

  sql << "SELECT ";

  // This is actually faster than '*' and guarantees the correct order
  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql << ", ";
    sql << '"' << it->getName() << '"';
  }

  sql << " FROM " << getEscapedName() << " " << suffix;

  return db.compile(sql.str());
}
