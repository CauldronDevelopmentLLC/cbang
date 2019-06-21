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

#pragma once

#include "ValueType.h"
#include "Factory.h"
#include "Writer.h"

#include <cbang/Errors.h>

#include <ostream>
#include <list>


namespace cb {
  namespace JSON {
    class Value;
    class Sink;

    class Value : public Factory, public ValueType::Enum {
    public:
      virtual ~Value() {}

      virtual ValueType getType() const = 0;
      virtual ValuePtr copy(bool deep = false) const = 0;
      virtual bool canWrite(Sink &sink) const {return true;}

      virtual bool isSimple() const {return true;}

      bool isUndefined() const {return getType() == JSON_UNDEFINED;}
      bool isNull()      const {return getType() == JSON_NULL;}
      bool isBoolean()   const {return getType() == JSON_BOOLEAN;}
      bool isNumber()    const {return getType() == JSON_NUMBER;}
      bool isString()    const {return getType() == JSON_STRING;}
      bool isList()      const {return getType() == JSON_LIST;}
      bool isDict()      const {return getType() == JSON_DICT;}

      virtual bool isS8()  const {return false;}
      virtual bool isU8()  const {return false;}
      virtual bool isS16() const {return false;}
      virtual bool isU16() const {return false;}
      virtual bool isS32() const {return false;}
      virtual bool isU32() const {return false;}
      virtual bool isS64() const {return false;}
      virtual bool isU64() const {return false;}

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

      ValuePtr select(const std::string &path) const;

#define CBANG_JSON_SELECT(NAME, TYPE)                               \
      TYPE select##NAME(const std::string &path) const {            \
        ValuePtr value = select(path);                              \
        try {                                                       \
          return value->get##NAME();                                \
        } catch (const Exception &e) {                              \
          CBANG_THROWC("At JSON path " << path, e);                 \
        }                                                           \
      }                                                             \
      TYPE select##NAME(const std::string &path,                    \
                        TYPE defaultValue) const {                  \
        try {                                                       \
          return select##NAME(path);                                \
        } catch (...) {return defaultValue;}                        \
      }

      CBANG_JSON_SELECT(Boolean, bool);
      CBANG_JSON_SELECT(Number,  double);
      CBANG_JSON_SELECT(String,  const std::string &);
      CBANG_JSON_SELECT(List,    Value &);
      CBANG_JSON_SELECT(Dict,    Value &);

      CBANG_JSON_SELECT(S8,      int8_t);
      CBANG_JSON_SELECT(U8,      uint8_t);
      CBANG_JSON_SELECT(S16,     int16_t);
      CBANG_JSON_SELECT(U16,     uint16_t);
      CBANG_JSON_SELECT(S32,     int32_t);
      CBANG_JSON_SELECT(U32,     uint32_t);
      CBANG_JSON_SELECT(S64,     int64_t);
      CBANG_JSON_SELECT(U64,     uint64_t);

#undef CBANG_JSON_SELECT

#define CBANG_JSON_GET(NAME, TYPE)                                      \
      virtual TYPE get##NAME() const {CBANG_TYPE_ERROR("Not a " #NAME);}

      CBANG_JSON_GET(Boolean, bool);
      CBANG_JSON_GET(Number,  double);
      CBANG_JSON_GET(String,  const std::string &);
      CBANG_JSON_GET(List,    const Value &);
      CBANG_JSON_GET(Dict,    const Value &);

      CBANG_JSON_GET(S8,      int8_t);
      CBANG_JSON_GET(U8,      uint8_t);
      CBANG_JSON_GET(S16,     int16_t);
      CBANG_JSON_GET(U16,     uint16_t);
      CBANG_JSON_GET(S32,     int32_t);
      CBANG_JSON_GET(U32,     uint32_t);
      CBANG_JSON_GET(S64,     int64_t);
      CBANG_JSON_GET(U64,     uint64_t);

#undef CBANG_JSON_GET

      virtual Value &getList() {CBANG_TYPE_ERROR("Not a List");}
      virtual Value &getDict() {CBANG_TYPE_ERROR("Not a Dict");}

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
      bool getBoolean(unsigned i) const {return get(i)->getBoolean();}
      double getNumber(unsigned i) const {return get(i)->getNumber();}
      int8_t getS8(unsigned i) const {return get(i)->getS8();}
      uint8_t getU8(unsigned i) const {return get(i)->getU8();}
      int16_t getS16(unsigned i) const {return get(i)->getS16();}
      uint16_t getU16(unsigned i) const {return get(i)->getU16();}
      int32_t getS32(unsigned i) const {return get(i)->getS32();}
      uint32_t getU32(unsigned i) const {return get(i)->getU32();}
      int64_t getS64(unsigned i) const {return get(i)->getS64();}
      uint64_t getU64(unsigned i) const {return get(i)->getU64();}
      const std::string &getString(unsigned i) const
      {return get(i)->getString();}
      std::string getAsString(unsigned i) const {return get(i)->asString();}
      Value &getList(unsigned i) {return get(i)->getList();}
      const Value &getList(unsigned i) const {return get(i)->getList();}
      Value &getDict(unsigned i) {return get(i)->getDict();}
      const Value &getDict(unsigned i) const {return get(i)->getDict();}

      // List setters
      virtual void set(unsigned i, const ValuePtr &value)
        {CBANG_TYPE_ERROR("Not a List");}
      void setDict(unsigned i);
      void setList(unsigned i);
      void setUndefined(unsigned i);
      void setNull(unsigned i);
      void setBoolean(unsigned i, bool value);
      void set(unsigned i, double value);
      void set(unsigned i, float value) {set(i, (double)value);}
      void set(unsigned i, int8_t value) {set(i, (int16_t)value);}
      void set(unsigned i, uint8_t value) {set(i, (uint16_t)value);}
      void set(unsigned i, int16_t value) {set(i, (int32_t)value);}
      void set(unsigned i, uint16_t value) {set(i, (uint32_t)value);}
      void set(unsigned i, int32_t value) {set(i, (int64_t)value);}
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

#define CBANG_JSON_INDEX_OF(TYPE)                                       \
      int indexOf##TYPE(const std::string &key) const {                 \
        int index = indexOf(key);                                       \
        return (index != -1 && get(index)->is##TYPE()) ? index : -1;    \
      }

      CBANG_JSON_INDEX_OF(Null);
      CBANG_JSON_INDEX_OF(Boolean);
      CBANG_JSON_INDEX_OF(Number);
      CBANG_JSON_INDEX_OF(String);
      CBANG_JSON_INDEX_OF(List);
      CBANG_JSON_INDEX_OF(Dict);

      CBANG_JSON_INDEX_OF(S8);
      CBANG_JSON_INDEX_OF(U8);
      CBANG_JSON_INDEX_OF(S16);
      CBANG_JSON_INDEX_OF(U16);
      CBANG_JSON_INDEX_OF(S32);
      CBANG_JSON_INDEX_OF(U32);
      CBANG_JSON_INDEX_OF(S64);
      CBANG_JSON_INDEX_OF(U64);

#undef CBANG_JSON_INDEX_OF

      bool has(const std::string &key) const {return indexOf(key) != -1;}

#define CBANG_JSON_HAS_KEY(TYPE)                                \
      bool has##TYPE(const std::string &key) const {            \
        return indexOf##TYPE(key) != -1;                        \
      }

      CBANG_JSON_HAS_KEY(Null);
      CBANG_JSON_HAS_KEY(Boolean);
      CBANG_JSON_HAS_KEY(Number);
      CBANG_JSON_HAS_KEY(String);
      CBANG_JSON_HAS_KEY(List);
      CBANG_JSON_HAS_KEY(Dict);

      CBANG_JSON_HAS_KEY(S8);
      CBANG_JSON_HAS_KEY(U8);
      CBANG_JSON_HAS_KEY(S16);
      CBANG_JSON_HAS_KEY(U16);
      CBANG_JSON_HAS_KEY(S32);
      CBANG_JSON_HAS_KEY(U32);
      CBANG_JSON_HAS_KEY(S64);
      CBANG_JSON_HAS_KEY(U64);

#undef CBANG_JSON_HAS_KEY

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
#define CBANG_JSON_GET(NAME, TYPE)                              \
      TYPE get##NAME(const std::string &key) const {            \
        return get(key)->get##NAME();                           \
      }

      CBANG_JSON_GET(Boolean, bool);
      CBANG_JSON_GET(Number,  double);
      CBANG_JSON_GET(String,  const std::string &);
      CBANG_JSON_GET(List,    const Value &);
      CBANG_JSON_GET(Dict,    const Value &);

      CBANG_JSON_GET(S8,      int8_t);
      CBANG_JSON_GET(U8,      uint8_t);
      CBANG_JSON_GET(S16,     int16_t);
      CBANG_JSON_GET(U16,     uint16_t);
      CBANG_JSON_GET(S32,     int32_t);
      CBANG_JSON_GET(U32,     uint32_t);
      CBANG_JSON_GET(S64,     int64_t);
      CBANG_JSON_GET(U64,     uint64_t);

#undef CBANG_JSON_GET

      std::string getAsString(const std::string &key) const
      {return get(key)->asString();}
      Value &getList(const std::string &key) {return get(key)->getList();}
      Value &getDict(const std::string &key) {return get(key)->getDict();}

      // Dict accessors with defaults
#define CBANG_JSON_GET_DEFAULT(NAME, TYPE)                              \
      TYPE get##NAME(const std::string &key,                            \
                     const TYPE &defaultValue) const {                  \
        int index = indexOf##NAME(key);                                 \
        return index == -1 ? defaultValue : get(index)->get##NAME();    \
      }

      CBANG_JSON_GET_DEFAULT(Boolean, bool);
      CBANG_JSON_GET_DEFAULT(Number,  double);
      CBANG_JSON_GET_DEFAULT(String,  std::string);

      CBANG_JSON_GET_DEFAULT(S8,      int8_t);
      CBANG_JSON_GET_DEFAULT(U8,      uint8_t);
      CBANG_JSON_GET_DEFAULT(S16,     int16_t);
      CBANG_JSON_GET_DEFAULT(U16,     uint16_t);
      CBANG_JSON_GET_DEFAULT(S32,     int32_t);
      CBANG_JSON_GET_DEFAULT(U32,     uint32_t);
      CBANG_JSON_GET_DEFAULT(S64,     int64_t);
      CBANG_JSON_GET_DEFAULT(U64,     uint64_t);

#undef CBANG_JSON_GET_DEFAULT

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
                         const String::FormatCB &cb) const;
      std::string format(const std::string &s,
                         const String::FormatCB &cb) const;
      std::string format(const std::string &s,
                         const std::string &defaultValue = "") const;

      // Operators
      const ValuePtr &operator[](unsigned i) const {return get(i);}
      const ValuePtr &operator[](const std::string &key) const
        {return get(key);}

      virtual void write(Sink &sink) const = 0;
      std::string toString(unsigned indent = 0, bool compact = false) const;
      std::string asString() const;
    };

    static inline
    std::ostream &operator<<(std::ostream &stream, const Value &value) {
      Writer writer(stream);
      value.write(writer);
      return stream;
    }
  }
}
