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

#include "Context.h"

#include <cbang/js/JSInterrupted.h>

using namespace cb;
using namespace cb::gv8;
using namespace std;


Context::Context(v8::Isolate *iso) : context(v8::Context::New(iso)) {}


Value Context::eval(const InputSource &src) {
  v8::EscapableHandleScope handleScope(Value::getIso());
  Context::Scope ctxScope(*this);

  // Get script origin
  v8::Local<v8::String> origin;
  string filename = src.getName();
  if (!filename.empty()) origin = Value::createString(filename);

#if V8_MAJOR_VERSION < 10
  v8::ScriptOrigin sOrigin(origin);
#else
  v8::ScriptOrigin sOrigin(Value::getIso(), origin);
#endif

  // Get script source
  auto source = Value::createString(src.toString());

  // Compile
  v8::TryCatch tryCatch(Value::getIso());
  auto script = v8::Script::Compile(context, source, &sOrigin);
  if (tryCatch.HasCaught()) translateException(tryCatch, false);

  // Execute
  auto ret = script.ToLocalChecked()->Run(context);
  if (tryCatch.HasCaught()) translateException(tryCatch, true);

  // Return result
  return handleScope.Escape(ret.ToLocalChecked());
}


void Context::translateException(const v8::TryCatch &tryCatch, bool useStack) {
  v8::HandleScope handleScope(Value::getIso());

  if (useStack && !tryCatch.StackTrace(context).IsEmpty())
    throw Exception(Value(tryCatch.StackTrace(context)).toString());

  if (tryCatch.Exception()->IsNull()) throw cb::js::JSInterrupted();

  string msg = Value(tryCatch.Exception()).toString();

  v8::Handle<v8::Message> message = tryCatch.Message();
  if (message.IsEmpty()) throw Exception(msg);

  string filename = Value(message->GetScriptResourceName()).toString();
  int line = message->GetLineNumber(context).FromJust();
  int col = message->GetStartColumn(context).FromJust();

  throw Exception(msg, FileLocation(filename, line, col));
}
