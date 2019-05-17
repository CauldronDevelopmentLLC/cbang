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

#include "V8.h"

#include <cbang/Exception.h>
#include <cbang/js/Value.h>
#include <cbang/js/Function.h>

#include <ostream>
#include <vector>


namespace cb {
  namespace gv8 {
    class Value : public js::Value {
    protected:
      v8::Handle<v8::Value> value;

    public:
      Value() : value(v8::Undefined(v8::Isolate::GetCurrent())) {}
      Value(const SmartPointer<js::Value> &value);
      Value(const js::Value &value);
      Value(const js::Function &func);
      Value(const Value &value);
      Value(const v8::Handle<v8::Primitive> &value) : value(value) {}
      Value(const v8::Local<v8::Object> &value) : value(value) {}
      Value(const v8::Local<v8::Array> &value) : value(value) {}
      Value(const v8::Handle<v8::Value> &value);
      Value(const v8::MaybeLocal<v8::Value> &value);
      Value(const v8::MaybeLocal<v8::Array> &value);
      explicit Value(bool x) : value(x ? v8::True(v8::Isolate::GetCurrent()) :
                                     v8::False(v8::Isolate::GetCurrent())) {}
      Value(double x) : value(v8::Number::New(v8::Isolate::GetCurrent(), x)) {}
      Value(int32_t x) : value(v8::Int32::New(v8::Isolate::GetCurrent(), x)) {}
      Value(uint32_t x) :
        value(v8::Uint32::New(v8::Isolate::GetCurrent(), x)) {}
      Value(const char *s, int length = -1);
      Value(const std::string &s);

      SmartPointer<js::Value> makePersistent() const;

      // Undefined
      void assertDefined() const
      {if (isUndefined()) CBANG_THROW("Value is undefined");}
      bool isUndefined() const {return value->IsUndefined();}

      // Null
      bool isNull() const {return value->IsNull();}
      static Value createNull() {return v8::Null(v8::Isolate::GetCurrent());}

      // Boolean
      void assertBoolean() const
      {if (!isBoolean()) CBANG_THROW("Value is not a boolean");}
      bool isBoolean() const {return value->IsBoolean();}
      bool toBoolean() const {assertDefined(); return value->BooleanValue();}

      // Number
      void assertNumber() const
      {if (!isNumber()) CBANG_THROW("Value is not a number");}
      bool isNumber() const {return value->IsNumber();}
      double toNumber() const {assertDefined(); return value->NumberValue();}
      int toInteger() const {assertDefined(); return value->IntegerValue();}

      // Int32
      void assertInt32() const
      {if (!isInt32()) CBANG_THROW("Value is not a int32");}
      bool isInt32() const {return value->IsInt32();}
      int32_t toInt32() const {assertDefined(); return value->Int32Value();}

      // Uint32
      void assertUint32() const
      {if (!isUint32()) CBANG_THROW("Value is not a uint32");}
      bool isUint32() const {return value->IsUint32();}
      uint32_t toUint32() const {assertDefined(); return value->Uint32Value();}

      // String
      static v8::Local<v8::String> createString(const char *s, unsigned length);
      static v8::Local<v8::String> createString(const std::string &s);
      static v8::Local<v8::Symbol> createSymbol(const std::string &name);
      void assertString() const
      {if (!isString()) CBANG_THROW("Value is not a string");}
      bool isString() const {return value->IsString();}
      std::string toString() const;

      int utf8Length() const {return v8::String::Cast(*value)->Utf8Length();}

      // Object
      static Value createObject()
      {return v8::Object::New(v8::Isolate::GetCurrent());}
      void assertObject() const
      {if (!isObject()) CBANG_THROW("Value is not a object");}
      bool isObject() const {return value->IsObject();}
      bool has(uint32_t index) const;
      bool has(const std::string &key) const;

      SmartPointer<js::Value> get(int index) const;
      SmartPointer<js::Value> get(const std::string &key) const;
      SmartPointer<js::Value> getOwnPropertyNames() const;

      void set(int index, const js::Value &value);
      void set(const std::string &key, const js::Value &value);

      // Array
      static Value createArray(unsigned size = 0)
      {return v8::Array::New(v8::Isolate::GetCurrent(), size);}
      void assertArray() const
      {if (!isArray()) CBANG_THROW("Value is not a array");}
      bool isArray() const {return value->IsArray();}
      unsigned length() const;

      // Function
      bool isFunction() const {return value->IsFunction();}
      Value call(Value arg0, const std::vector<Value> &args) const;
      SmartPointer<js::Value>
      call(const std::vector<SmartPointer<js::Value> > &args) const;
      std::string getName() const;
      void setName(const std::string &name);
      int getScriptLineNumber() const;

      // Accessors
      const v8::Handle<v8::Value> &getV8Value() const {return value;}
      v8::Handle<v8::Value> &getV8Value() {return value;}

      // Static
      static v8::Isolate *getIso() {return v8::Isolate::GetCurrent();}
      static v8::Local<v8::Context> getCtx()
        {return getIso()->GetCurrentContext();}

       static void throwError(const std::string &msg);
    };
  }
}
