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

#include "Value.h"
#include "ValueRef.h"
#include "Factory.h"
#include "JSImpl.h"

#include <cbang/js/Callback.h>
#include <cbang/js/Sink.h>

using namespace cb::gv8;
using namespace cb;
using namespace std;


namespace {
  void _callback(const v8::FunctionCallbackInfo<v8::Value> &info) {
    js::Callback &cb =
      *static_cast<js::Callback *>(v8::External::Cast(*info.Data())->Value());

    try {
      // Convert args
      Value args = Value::createArray(info.Length());
      for (int i = 0; i < info.Length(); i++)
        args.set(i, Value(info[i]));

      // Call function
      auto ret = cb.call(args);
      info.GetReturnValue().Set(ret.cast<Value>()->getV8Value());

    } catch (const Exception &e) {Value::throwError(SSTR(e));}
    catch (const std::exception &e) {Value::throwError(e.what());}
    catch (...) {Value::throwError("Unknown exception");}
  }
}


Value::Value(const SmartPointer<js::Value> &value) :
  value(value.cast<Value>()->getV8Value()) {}


Value::Value(const js::Value &value) {
  const Value *v = dynamic_cast<const Value *>(&value);
  if (!v) THROW("Not a gv8::Value");
  this->value = v->getV8Value();
}


Value::Value(const js::Function &func) {
  SmartPointer<js::Callback> cb = func.getCallback();
  JSImpl::current().add(cb);

  v8::Handle<v8::Value> data =
    v8::External::New(getIso(), (void *)cb.get());
  v8::Handle<v8::FunctionTemplate> tmpl =
    v8::FunctionTemplate::New(getIso(), &_callback, data);
  value = tmpl->GetFunction();
}


Value::Value(const Value &value) {
  if (value.getV8Value().IsEmpty()) this->value = v8::Undefined(getIso());
  else this->value = value.getV8Value();
}


Value::Value(const v8::Handle<v8::Value> &value) {
  if (value.IsEmpty()) this->value = v8::Undefined(getIso());
  else this->value = value;
}


Value::Value(const v8::MaybeLocal<v8::Value> &value) {
  if (!value.ToLocal(&this->value)) this->value = v8::Undefined(getIso());
}


Value::Value(const v8::MaybeLocal<v8::Array> &value) {
  if (!value.ToLocal(&this->value)) this->value = v8::Undefined(getIso());
}


Value::Value(const char *s, int length) : value(createString(s, length)) {}
Value::Value(const std::string &s) : value(createString(s)) {}



SmartPointer<js::Value> Value::makePersistent() const {
  return new ValueRef(*this);
}


v8::Local<v8::String> Value::createString(const char *s, unsigned length) {
  return v8::String::NewFromUtf8
    (getIso(), s, v8::NewStringType::kNormal, length).ToLocalChecked();
}


v8::Local<v8::String> Value::createString(const string &s) {
  return createString(s.c_str(), s.length());
}


v8::Local<v8::Symbol> Value::createSymbol(const string &name) {
  return v8::Symbol::New(getIso(), createString(name));
}


string Value::toString() const {
  if (isUndefined()) return "undefined";
  if (isFunction()) return SSTR("function " << getName() << "() {...}");

  // TODO this is not very efficient
  v8::String::Utf8Value s(value);
  return *s ? string(*s, s.length()) : "(null)";
}


bool Value::has(uint32_t index) const {
  return value->ToObject()->Has(getCtx(), index).ToChecked();
}


bool Value::has(const string &key) const {
  return value->ToObject()->Has(getCtx(), createString(key)).ToChecked();
}


SmartPointer<js::Value> Value::get(int index) const {
  return new Value(value->ToObject()->Get(getCtx(), index));
}


SmartPointer<js::Value> Value::get(const string &key) const {
  return new Value(value->ToObject()->Get(getCtx(), createString(key)));
}


SmartPointer<js::Value> Value::getOwnPropertyNames() const {
  return new Value(value->ToObject()->GetOwnPropertyNames(getCtx()));
}


void Value::set(int index, const js::Value &value) {
  v8::Maybe<bool> ret = this->value->ToObject()->
    Set(getCtx(), index, Value(value).getV8Value());
  if (ret.IsNothing() || !ret.ToChecked())
    THROW("Set " << index << " failed");
}


void Value::set(const string &key, const js::Value &value) {
  v8::Maybe<bool> ret = this->value->ToObject()->
    Set(getCtx(), createString(key), Value(value).getV8Value());
  if (ret.IsNothing() || !ret.ToChecked())
    THROW("Set " << key << " failed");
}


unsigned Value::length() const {
  if (isString()) return v8::String::Cast(*value)->Length();
  else if (isArray()) return v8::Array::Cast(*value)->Length();
  else if (isObject()) return getOwnPropertyNames()->length();
  THROW("Value does not have length");
}


Value Value::call(Value arg0, const vector<Value> &args) const {
  if (!isFunction()) THROW("Value is not a function");

  SmartPointer<v8::Handle<v8::Value> >::Array argv =
    new v8::Handle<v8::Value>[args.size()];

  for (unsigned i = 0; i < args.size(); i++)
    argv[i] = args[i].getV8Value();

  return v8::Handle<v8::Function>::Cast(value)->
    Call(arg0.getV8Value()->ToObject(), args.size(), argv.get());

  return 0;
}


SmartPointer<js::Value>
Value::call(const vector<SmartPointer<js::Value> > &_args) const {
  vector<Value> args;
  for (unsigned i = 0; i < _args.size(); i++)
    args.push_back(Value(*_args[i]));

  return new Value(call(*this, args));
}


string Value::getName() const {
  if (!isFunction()) THROW("Value is not a function");
  return Value(v8::Handle<v8::Function>::Cast(value)->GetName()).
    toString();
}


void Value::setName(const string &name) {
  if (!isFunction()) THROW("Value is not a function");
  v8::Handle<v8::Function>::Cast(value)->SetName(createString(name));
}


int Value::getScriptLineNumber() const {
  if (!isFunction()) THROW("Value is not a function");
  return v8::Handle<v8::Function>::Cast(value)->GetScriptLineNumber();
}


void Value::throwError(const string &msg) {
  getIso()->ThrowException(v8::Exception::Error(createString(msg)));
}
