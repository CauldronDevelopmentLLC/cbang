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

#pragma once

#include "Column.h"
#include "Statement.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <functional>
#include <cstdint>


namespace cb {
  namespace JSON {class Value;}

  namespace DB {
    class Database;

    class NameValueTable {
      Database &db;
      const std::string table;
      bool ordered;

      SmartPointer<Statement> replaceStmt;
      SmartPointer<Statement> deleteStmt;
      SmartPointer<Statement> selectStmt;
      SmartPointer<Statement> foreachStmt;

    public:
      NameValueTable(
        Database &db, const std::string &table, bool ordered = false);

      void init();
      void create();

      [[gnu::format(printf, 3, 4)]]
      void setf(const char *name, const char *value, ...);
      void set(const std::string &name, const std::string &value);
      void set(const std::string &name, int64_t value);
      void set(const std::string &name, uint64_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, int32_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, uint32_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, int16_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, uint16_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, int8_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, uint8_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, double value);
      void set(const std::string &name, float value)
      {set(name, (double)value);}
      void set(const std::string &name, bool value);
      void set(const std::string &name);

      void unset(const std::string &name);

      template <typename T>
      void set(const std::string &name, const T &value) {
        set(name, value.toString());
      }

      const std::string getString(const std::string &name) const;
      int64_t getInteger(const std::string &name) const;
      double getDouble(const std::string &name) const;
      bool getBoolean(const std::string &name) const;
      SmartPointer<JSON::Value> getJSON(const std::string &name) const;

      const std::string getString(const std::string &name,
                                  const std::string &defaultValue) const;
      int64_t getInteger(const std::string &name, int64_t defaultValue) const;
      double getDouble(const std::string &name, double defaultValue) const;
      bool getBoolean(const std::string &name, bool defaultValue) const;
      SmartPointer<JSON::Value>
      getJSON(const std::string &name,
              const SmartPointer<JSON::Value> &defaultValue) const;

      bool has(const std::string &name) const;

      void foreach(std::function<void (const std::string &name,
                                       const std::string &value)> cb,
                                       unsigned limit);

    protected:
      Column doGet(const std::string &name) const;
      void doSet(const std::string &name);
    };
  }
}
