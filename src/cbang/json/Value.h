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

#include "ValueType.h"
#include "Factory.h"
#include "Writer.h"
#include "Path.h"
#include "Iterator.h"
#include "KeyIterator.h"
#include "EntryIterator.h"

#include <cbang/SmartPointer.h>
#include <cbang/Errors.h>
#include <cbang/util/MacroUtils.h>

#include <ostream>
#include <list>
#include <functional>


namespace cb {
  namespace JSON {
    class Value;
    class Sink;

    class Value :
      virtual public RefCounted, public Factory, public ValueType::Enum {
    public:
      virtual ~Value() {}

      virtual ValueType getType() const = 0;

      virtual bool isSimple() const {return true;}
      virtual bool isInteger() const {return false;}
      virtual ValuePtr copy(bool deep = false) const = 0;
      virtual bool canWrite(Sink &sink) const {return true;}

      bool     exists(const std::string &path) const;
      ValuePtr select(const std::string &path) const;
      ValuePtr select(const std::string &path,
                      const ValuePtr &defaultValue) const;

      // X-Macros
#define CBANG_JSON_VT(NAME, TYPE, ...)                                   \
      virtual bool is##NAME() const {return false;}                      \
                                                                         \
      bool exists##NAME(const std::string &path) const                   \
      {return Path(path).exists##NAME(*this);}                           \
                                                                         \
      TYPE select##NAME(const std::string &path) const                   \
      {return Path(path).select##NAME(*this);}                           \
                                                                         \
      TYPE select##NAME(const std::string &path,                         \
                        TYPE defaultValue) const                         \
      {return Path(path).select##NAME(*this, defaultValue);}             \
                                                                         \
      virtual TYPE get##NAME() const {CBANG_TYPE_ERROR("Not a " #NAME);} \
                                                                         \
      virtual TYPE get##NAME##WithDefault(TYPE defaultValue) const {     \
        try {return get##NAME();}                                        \
        catch (...) {return defaultValue;}                               \
      }
#include "ValueTypes.def"

      // Non-const accessors
      virtual Value &getList() {CBANG_TYPE_ERROR("Not a List");}
      virtual Value &getDict() {CBANG_TYPE_ERROR("Not a Dict");}

      virtual bool toBoolean() const {return false;}
      virtual unsigned size() const {CBANG_TYPE_ERROR("Not a List or Dict");}
      bool empty() const {return !size();}

      // Iterators
      using iterator = Iterator;
      virtual iterator begin() const {CBANG_TYPE_ERROR("Not a List or Dict");}
      virtual iterator end()   const {CBANG_TYPE_ERROR("Not a List or Dict");}
      virtual iterator erase(const iterator &it)
        {CBANG_TYPE_ERROR("Not a List or Dict");}

      struct Keys {
        Value::iterator _begin, _end;
        Keys(const Value &value) : _begin(value.begin()), _end(value.end())  {}
        KeyIterator begin() {return _begin;}
        KeyIterator end()   {return _end;}
      };

      Keys keys() const {return Keys(*this);}

      struct Entries {
        Value::iterator _begin, _end;
        Entries(const Value &value) : _begin(value.begin()), _end(value.end())  {}
        EntryIterator begin() {return _begin;}
        EntryIterator end()   {return _end;}
      };

      Entries entries() const {return Entries(*this);}

      // List functions
      virtual iterator find(unsigned i) const {CBANG_TYPE_ERROR("Not a List");}
      virtual const ValuePtr &get(unsigned i) const
      {CBANG_TYPE_ERROR("Not a List");}
      Value &getList(unsigned i) {return get(i)->getList();}
      Value &getDict(unsigned i) {return get(i)->getDict();}
      std::string getAsString(unsigned i) const {return get(i)->asString();}

      virtual void set(unsigned i, const ValuePtr &value)
      {CBANG_TYPE_ERROR("Not a List");}
      virtual void append(const ValuePtr &value)
      {CBANG_TYPE_ERROR("Not a List");}
      void appendFrom(const Value &value);
      virtual void clear() {CBANG_TYPE_ERROR("Not a List or Dict");}
      virtual void erase(unsigned i) {CBANG_TYPE_ERROR("Not a List");}

      // List X-Macros
#define CBANG_JSON_VT(NAME, TYPE, PARAM, SUFFIX)                             \
      void append##SUFFIX(CBANG_IF(PARAM)(TYPE value))                       \
      {append(create##SUFFIX(CBANG_IF(PARAM)(value)));}                      \
                                                                             \
      TYPE get##NAME(unsigned i) const {return get(i)->get##NAME();}         \
                                                                             \
      void set##SUFFIX(unsigned i CBANG_IF(PARAM)(CBANG_COMMA() TYPE value)) \
        {set(i, create##SUFFIX(CBANG_IF(PARAM)(value)));}
#include "ValueTypes.def"

      // Dict functions
      bool has(const std::string &key) const {return find(key);}
      virtual iterator find(const std::string &key) const
      {CBANG_TYPE_ERROR("Not a Dict");}
      virtual const ValuePtr &get(const std::string &key) const
      {CBANG_TYPE_ERROR("Not a Dict");}

      const ValuePtr &get(const std::string &key,
                          const ValuePtr &defaultValue) const {
        auto it = find(key);
        return it ? *it : defaultValue;
      }

      std::string getAsString(const std::string &key) const
      {return get(key)->asString();}

      std::string getAsString(const std::string &key,
                              const std::string &defaultValue) const {
        auto it = find(key);
        return it ? (*it)->asString() : defaultValue;
      }

      Value &getList(const std::string &key) {return get(key)->getList();}
      Value &getDict(const std::string &key) {return get(key)->getDict();}

      virtual iterator insert(const std::string &key, const ValuePtr &value)
      {CBANG_TYPE_ERROR("Not a Dict");}
      virtual void erase(const std::string &key)
      {CBANG_TYPE_ERROR("Not a Dict");}
      void merge(const Value &value);

      // Dict X-Macros
#define CBANG_JSON_VT(NAME, TYPE, PARAM, SUFFIX)                                \
      bool has##NAME(const std::string &key) const {                    \
        auto it = find(key);                                            \
        return !!it && it.value()->is##NAME();                          \
      }                                                                 \
                                                                        \
      iterator insert##SUFFIX(const std::string &key                    \
        CBANG_IF(PARAM)(CBANG_COMMA() TYPE value))                      \
        {return insert(key, create##SUFFIX(CBANG_IF(PARAM)(value)));}   \
                                                                        \
      TYPE get##NAME(const std::string &key) const                      \
      {return get(key)->get##NAME();}                                   \
                                                                        \
      TYPE get##NAME(const std::string &key, TYPE defaultValue) const { \
        auto it = find(key);                                            \
        if (!it) return defaultValue;                                   \
        return (*it)->get##NAME##WithDefault(defaultValue);             \
      }
#include "ValueTypes.def"

      // Formatting
      std::string formatAs(const std::string &spec) const;
      std::string format(const std::string &fmt,
        const std::string &defaultValue) const;
      std::string format(const std::string &fmt) const;

      // Visitor
      typedef std::function<void (
        const Value &value, const Value *parent)> const_visitor_t;
      typedef std::function<void (Value &value, Value *parent)> visitor_t;

      void visit(const_visitor_t visitor, bool depthFirst = true) const;
      void visit(visitor_t visitor, bool depthFirst = true);

      virtual void visitChildren(
        const_visitor_t visitor, bool depthFirst = true) const {}
      virtual void visitChildren(visitor_t visitor, bool depthFirst = true) {}

      virtual void write(Sink &sink) const = 0;
      void write(std::ostream &stream, unsigned indentStart = 0,
                 bool compact = false, unsigned indentSpace = 2,
                 int precision = 6) const;

      std::string toString(unsigned indentStart = 0, bool compact = false,
                           unsigned indentSpace = 2, int precision = 6) const;
      std::string asString() const;

      // Comparison
      int compare(const Value &o) const;
      bool operator==(const Value &o) const {return compare(o) == 0;}
      bool operator!=(const Value &o) const {return compare(o) != 0;}
      bool operator< (const Value &o) const {return compare(o) <  0;}
      bool operator<=(const Value &o) const {return compare(o) <= 0;}
      bool operator> (const Value &o) const {return compare(o) >  0;}
      bool operator>=(const Value &o) const {return compare(o) >= 0;}
    };

    static inline
    std::ostream &operator<<(std::ostream &stream, const Value &value) {
      return stream << value.toString();
    }
  }
}
