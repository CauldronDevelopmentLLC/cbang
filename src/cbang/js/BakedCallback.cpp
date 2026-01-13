/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include "BakedCallback.h"
#include "Sink.h"

#include <cbang/log/Logger.h>

using namespace cb::js;
using namespace cb;
using namespace std;


SmartPointer<Value> BakedCallback::call(Callback &cb, Value &args) {
  // Process args
  SmartPointer<Value> newArgs;

  if (args.length() == 1 && args[0]->isObject()) newArgs = args[0];

  else if (sig.size()) {
    newArgs = factory->createObject();

    int index = 0;
    auto sigKeyIt = sig.begin();
    for (unsigned i = 0; i < args.length(); i++)
      if (sigKeyIt != sig.end()) newArgs->set((sigKeyIt++).key(), args[i]);
      else {
        while (newArgs->has(String(index))) index++;
        newArgs->set(index, args[i]);
      }

  } else newArgs = SmartPointer<Value>::Phony(&args);

  // Fill in defaults
  for (auto e: sig.entries()) {
    if (!newArgs->has(e.key()))
      switch (e->getType()) {
      case JSON::Value::JSON_NULL:
        newArgs->set(e.key(), factory->createNull());
        break;
      case JSON::Value::JSON_BOOLEAN:
        newArgs->set(e.key(), factory->createBoolean(e->getBoolean()));
        break;
      case JSON::Value::JSON_NUMBER:
        newArgs->set(e.key(), factory->create(e->getNumber()));
        break;
      case JSON::Value::JSON_STRING:
        newArgs->set(e.key(), factory->create(e->getString()));
        break;
      case JSON::Value::JSON_UNDEFINED: break;
      default: THROW("Unexpected type " << e->getType()
          << " in function signature");
      }
  }

  // Call function
  Sink sink(getFactory());
  (*this)(*newArgs, sink);

  // Make sure sink calls concluded correctly
  sink.close();

  if (sink.getRoot().isNull()) return getFactory()->createUndefined();
  return sink.getRoot();
}
