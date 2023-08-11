/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include <cbang/SmartPointer.h>
#include <cbang/Errors.h>

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

#define CBANG_JSON_VT(NAME, TYPE) virtual bool is##NAME() const {return false;}
#include "ValueTypes.def"

      virtual bool isSimple() const {return true;}
      template <typename T> bool is() {return dynamic_cast<T *>(this);}

      template <typename T> T &cast() {
        T *ptr = dynamic_cast<T *>(this);
        if (!ptr) CBANG_CAST_ERROR();
        return *ptr;
      }

      template <typename T> const T &cast() const {
        const T *ptr = dynamic_cast<T *>(this);
        if (!ptr) CBANG_CAST_ERROR();
        return *ptr;
      }

      virtual ValuePtr copy(bool deep = false) const = 0;
      virtual bool canWrite(Sink &sink) const {return true;}

      bool     exists(const std::string &path) const;
      ValuePtr select(const std::string &path) const;
      ValuePtr select(const std::string &path,
                      const ValuePtr &defaultValue) const;

#define CBANG_JSON_VT(NAME, TYPE)                                       \
      bool exists##NAME(const std::string &path) const {                \
        return Path(path).exists##NAME(*this);                          \
      }                                                                 \
                                                                        \
                                                                        \
      TYPE select##NAME(const std::string &path) const {                \
        return Path(path).select##NAME(*this);                          \
      }                                                                 \
                                                                        \
                                                                        \
      TYPE select##NAME(const std::string &path,                        \
                        TYPE defaultValue) const {                      \
        return Path(path).select##NAME(*this, defaultValue);            \
      }
#include "ValueTypes.def"


#define CBANG_JSON_VT(NAME, TYPE)                                       \
      virtual TYPE get##NAME() const {CBANG_TYPE_ERROR("Not a " #NAME);}
#include "ValueTypes.def"

#define CBANG_JSON_VT(NAME, TYPE)                                       \
      virtual TYPE get##NAME##WithDefault(TYPE defaultValue) const {    \
        try {return get##NAME();}                                       \
        catch (...) {return defaultValue;}                              \
      }
#include "ValueTypes.def"

      virtual Value &getList() {CBANG_TYPE_ERROR("Not a List");}
      virtual Value &getDict() {CBANG_TYPE_ERROR("Not a Dict");}

      virtual bool toBoolean() const {return false;}
      virtual unsigned size() const {CBANG_TYPE_ERROR("Not a List or Dict");}
      bool empty() const {return !size();}

      // List functions
      void appendDict();
      void appendList();
      void appendUndefined();
      void appendNull();
      void appendBoolean(bool value);
      void append(double value);
      void append(float value)    {append((double)value);}
      void append(int8_t value)   {append((int16_t)value);}
      void append(uint8_t value)  {append((uint16_t)value);}
      void append(int16_t value)  {append((int32_t)value);}
      void append(uint16_t value) {append((uint32_t)value);}
      void append(int32_t value)  {append((int64_t)value);}
      void append(uint32_t value) {append((uint64_t)value);}
      void append(int64_t value);
      void append(uint64_t value);
      void append(const std::string &value);
      void appendFrom(const Value &value);
      virtual void append(const ValuePtr &value)
        {CBANG_TYPE_ERROR("Not a List");}

      // List accessors
      virtual const ValuePtr &get(unsigned i) const
        {CBANG_TYPE_ERROR("Not a List or Dict");}

#define CBANG_JSON_VT(NAME, TYPE)               \
      TYPE get##NAME(unsigned i) const {return get(i)->get##NAME();}
#include "ValueTypes.def"

      Value &getList(unsigned i) {return get(i)->getList();}
      Value &getDict(unsigned i) {return get(i)->getDict();}
      std::string getAsString(unsigned i) const {return get(i)->asString();}

      // List setters
      virtual void set(unsigned i, const ValuePtr &value)
        {CBANG_TYPE_ERROR("Not a List");}
      void setDict(unsigned i);
      void setList(unsigned i);
      void setUndefined(unsigned i);
      void setNull(unsigned i);
      void setBoolean(unsigned i, bool value);
      void set(unsigned i, double value);
      void set(unsigned i, float value)    {set(i, (double)value);}
      void set(unsigned i, int8_t value)   {set(i, (int16_t)value);}
      void set(unsigned i, uint8_t value)  {set(i, (uint16_t)value);}
      void set(unsigned i, int16_t value)  {set(i, (int32_t)value);}
      void set(unsigned i, uint16_t value) {set(i, (uint32_t)value);}
      void set(unsigned i, int32_t value)  {set(i, (int64_t)value);}
      void set(unsigned i, uint32_t value) {set(i, (uint64_t)value);}
      void set(unsigned i, int64_t value);
      void set(unsigned i, uint64_t value);
      void set(unsigned i, const std::string &value);

      virtual void clear() {CBANG_TYPE_ERROR("Not a List or Dict");}
      virtual void erase(unsigned i) {CBANG_TYPE_ERROR("Not a List or Dict");}

      // Dict functions
      virtual const std::string &keyAt(unsigned i) const
        {CBANG_TYPE_ERROR("Not a Dict");}

      virtual int indexOf(const std::string &key) const
        {CBANG_TYPE_ERROR("Not a Dict");}

      bool has(const std::string &key) const {return indexOf(key) != -1;}

#define CBANG_JSON_VT(NAME, TYPE)                               \
      bool has##NAME(const std::string &key) const {            \
        int index = indexOf(key);                               \
        return index != -1 && get(index)->is##NAME();           \
      }
#include "ValueTypes.def"

      virtual const ValuePtr &get(const std::string &key) const
        {CBANG_TYPE_ERROR("Not a Dict");}

      virtual unsigned insert(const std::string &key, const ValuePtr &value)
        {CBANG_TYPE_ERROR("Not a Dict");}
      unsigned insertDict(const std::string &key);
      unsigned insertList(const std::string &key);
      unsigned insertUndefined(const std::string &key);
      unsigned insertNull(const std::string &key);
      unsigned insertBoolean(const std::string &key, bool value);
      unsigned insert(const std::string &key, double value);
      unsigned insert(const std::string &key, float value)
      {return insert(key, (double)value);}
      unsigned insert(const std::string &key, uint8_t value)
      {return insert(key, (uint16_t)value);}
      unsigned insert(const std::string &key, int8_t value)
      {return insert(key, (int16_t)value);}
      unsigned insert(const std::string &key, uint16_t value)
      {return insert(key, (uint32_t)value);}
      unsigned insert(const std::string &key, int16_t value)
      {return insert(key, (int32_t)value);}
      unsigned insert(const std::string &key, uint32_t value)
      {return insert(key, (uint64_t)value);}
      unsigned insert(const std::string &key, int32_t value)
      {return insert(key, (int64_t)value);}
      unsigned insert(const std::string &key, uint64_t value);
      unsigned insert(const std::string &key, int64_t value);
      unsigned insert(const std::string &key, const std::string &value);

      virtual void erase(const std::string &key)
        {CBANG_TYPE_ERROR("Not a Dict");}
      void merge(const Value &value);

      // Dict accessors
#define CBANG_JSON_VT(NAME, TYPE)                               \
      TYPE get##NAME(const std::string &key) const {            \
        return get(key)->get##NAME();                           \
      }
#include "ValueTypes.def"

      Value &getList(const std::string &key) {return get(key)->getList();}
      Value &getDict(const std::string &key) {return get(key)->getDict();}
      std::string getAsString(const std::string &key) const
      {return get(key)->asString();}

      // Dict accessors with defaults
#define CBANG_JSON_VT(NAME, TYPE)                                       \
      TYPE get##NAME(const std::string &key, TYPE defaultValue) const { \
        int index = indexOf(key);                                       \
        if (index == -1) return defaultValue;                           \
        return get(index)->get##NAME##WithDefault(defaultValue);        \
      }
#include "ValueTypes.def"

      const ValuePtr &get(const std::string &key,
                          const ValuePtr &defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index);
      }

      std::string getAsString(const std::string &key,
                              const std::string &defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->asString();
      }

      // Observable
      virtual void setParentRef(Value *parent, unsigned index) {}
      virtual void decParentRef() {}
      void clearParentRef() {setParentRef(0, 0);}
      virtual void notify(std::list<ValuePtr> &change)
      {CBANG_TYPE_ERROR("Not an Observable");}

      // Formatting
      std::string format(char type) const;
      std::string format(char type, int index, const std::string &name,
                         bool &matched) const;
      std::string format(const std::string &s,
                         const std::string &defaultValue) const;
      std::string format(const std::string &s) const;

      // Visitor
      typedef std::function<void (const Value &value, const Value *parent,
                                  unsigned index)> const_visitor_t;
      typedef std::function<void (Value &value, Value *parent, unsigned index)>
      visitor_t;

      void visit(const_visitor_t visitor, bool depthFirst = true) const;
      void visit(visitor_t visitor, bool depthFirst = true);

      virtual void visitChildren
      (const_visitor_t visitor, bool depthFirst = true) const {}
      virtual void visitChildren(visitor_t visitor, bool depthFirst = true) {}

      // Operators
      const ValuePtr &operator[](unsigned i) const {return get(i);}
      const ValuePtr &operator[](const std::string &key) const
        {return get(key);}

      virtual void write(Sink &sink) const = 0;
      void write(std::ostream &stream, unsigned indentStart = 0,
                 bool compact = false, unsigned indentSpace = 2,
                 int precision = 6) const;

      std::string toString(unsigned indentStart = 0, bool compact = false,
                           unsigned indentSpace = 2, int precision = 6) const;
      std::string asString() const;

      // Comparison
      bool operator==(const Value &o) const;
      bool operator!=(const Value &o) const {return !(*this == o);}
    };

    static inline
    std::ostream &operator<<(std::ostream &stream, const Value &value) {
      value.write(stream);
      return stream;
    }
  }
}
