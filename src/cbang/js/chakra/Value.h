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

#include <cbang/Exception.h>
#include <cbang/js/Value.h>
#include <cbang/js/Function.h>

#include <string>


namespace cb {
  namespace chakra {
    extern "C" typedef void *(*callback_t)
      (void *, bool, void **, unsigned short, void *);


    class Value : public js::Value {
    protected:
      void *ref;

    public:
      Value(void *ref = 0) : ref(ref) {}
      Value(const SmartPointer<js::Value> &value);
      Value(const std::string &value);
      Value(int value);
      explicit Value(double value);
      explicit Value(bool value);
      Value(const js::Function &func);
      Value(const std::string &name, callback_t cb, void *data);

      int getType() const;

      // From js::Value
      SmartPointer<js::Value> makePersistent() const override;

      bool isArray() const override;
      bool isBoolean() const override;
      bool isFunction() const override;
      bool isNull() const override;
      bool isNumber() const override;
      bool isObject() const override;
      bool isString() const override;
      bool isUndefined() const override;

      bool toBoolean() const override;
      int toInteger() const override;
      double toNumber() const override;
      std::string toString() const override;

      unsigned length() const override;
      SmartPointer<js::Value> get(int i) const override;

      bool has(const std::string &key) const override;
      SmartPointer<js::Value> get(const std::string &key) const override;

      void set(int i, const js::Value &value) override;
      void set(const std::string &key, const js::Value &value) override;

      void set(int i, const Value &value) override;
      void append(const Value &value);
      void set(const std::string &key, const Value &value,
               bool strict = false) override;
      SmartPointer<js::Value>
      call(const std::vector<SmartPointer<js::Value> > &args) const;

      SmartPointer<js::Value> getOwnPropertyNames() const;

      void setNull(const std::string &key);
      Value setObject(const std::string &key);
      Value setArray(const std::string &key, unsigned length = 0);

      operator void * () const {return ref;}

      static Value getGlobal();
      static Value getNull();
      static Value getUndefined();

      static bool hasException();
      static Value getException();

      static Value createArray(unsigned length = 0);
      static Value createArrayBuffer(unsigned length = 0, const char *data = 0);
      static Value createArrayBuffer(const std::string &s);
      static Value createObject();
      static Value createError(const std::string &msg);
      static Value createSyntaxError(const std::string &msg);

      static const char *errorToString(int error);
    };
  }
}


#define CHAKRA_CHECK(CMD)                                           \
  do {                                                              \
    JsErrorCode err = CMD;                                          \
    if (err != JsNoError)                                           \
      THROW(#CMD << " failed with 0x" << std::hex << err           \
             << ' ' << Value::errorToString(err));                  \
  } while (0)
