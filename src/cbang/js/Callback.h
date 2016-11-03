/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#ifndef CB_JS_CALLBACK_H
#define CB_JS_CALLBACK_H

#include "Value.h"
#include "Arguments.h"
#include "Signature.h"

#include "V8.h"


namespace cb {
  namespace js {
    class Callback {
      Signature sig;
      v8::Handle<v8::Value> data;
      v8::Handle<v8::FunctionTemplate> function;

    public:
      Callback(const Signature &sig);
      virtual ~Callback() {}

      const Signature getSignature() const {return sig;}

      virtual Value operator()(const Arguments &args) = 0;

      v8::Handle<v8::FunctionTemplate> getTemplate() const {return function;}

    protected:
      static void raise(const v8::FunctionCallbackInfo<v8::Value> &info,
                        const std::string &msg);
      static void callback(const v8::FunctionCallbackInfo<v8::Value> &info);
   };
  }
}

#endif // CB_JS_CALLBACK_H
