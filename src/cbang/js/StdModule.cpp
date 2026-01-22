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

#include "StdModule.h"
#include "Javascript.h"

using namespace cb::js;
using namespace cb;
using namespace std;


void StdModule::define(Sink &exports) {
  exports.insert("require(id)", this, &StdModule::require);
  exports.insert("readfile(path)", this, &StdModule::readfile);
  exports.insert("print(...)", this, &StdModule::print);
}


SmartPointer<Value> StdModule::require(Callback &cb, Value &args) {
  return js.require(cb, args);
}

void test(std::istream& stream) {
  printf("---test 1\n");
  int c = 0;
  while ((c = stream.get()) != EOF) {
    printf("--@test 2: %d\n", c);
    // printf("--@readfile 3: %d\n", c);
    // sink.append(c);
  }
}

void StdModule::readfile(const Value &args, Sink &sink) {
  // For some reason this needs to be done in two steps or `get` apparently segfaults.
  // std::istream& stream = InputSource::open(args.getString("path"));
  InputSource file = InputSource::open(args.getString("path"));
  std::istream& stream = file;
  
  sink.beginList();
  int c = 0;
  while ((c = stream.get()) != EOF) {
    sink.append(c);
  }
  sink.endList();
}

void StdModule::print(const Value &args, Sink &sink) {
  for (unsigned i = 0; i < args.length(); i++) {
    if (i) *stream << ' ';
    SmartPointer<Value> arg = args.get(i);
    if (arg->isObject() && !arg->isFunction()) *stream << js.stringify(*arg);
    else *stream << arg->toString();
  }
}
