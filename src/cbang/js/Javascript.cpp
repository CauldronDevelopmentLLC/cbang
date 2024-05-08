/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "Javascript.h"

#include "Value.h"

#include <cbang/config.h>

#ifdef HAVE_V8
#include <cbang/js/v8/JSImpl.h>
#endif

#include <cbang/util/SmartFunctor.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/json/JSON.h>
#include <cbang/util/SmartFunctor.h>

using namespace cb::js;
using namespace cb;
using namespace std;


Javascript::Javascript(const string &implName,
                       const SmartPointer<ostream> &stream) :
  impl(0), stdMod(*this, stream) {
#ifdef HAVE_V8
  if (implName == "v8" || (impl.isNull() && implName.empty()))
    impl = new gv8::JSImpl(*this);
#endif

  if (impl.isNull()) {
    if (implName.empty()) THROW("No Javscript implementation compiled in");
    else THROW("Javscript implementation '" << implName
                << "' not found in this build");
  }

  {
    SmartPointer<Scope> scope = impl->enterScope();
    nativeProps = getFactory()->createObject()->makePersistent();
  }

  define(stdMod);
  define(consoleMod);

  import("std", ".");
  import("console");
}


void Javascript::setStream(const SmartPointer<ostream> &stream) {
  stdMod.setStream(stream);
}


SmartPointer<js::Factory> Javascript::getFactory() {return impl->getFactory();}


void Javascript::define(NativeModule &mod) {
  SmartPointer<Scope> scope = impl->enterScope();

  // Create module
  SmartPointer<Module>::Phony module = &mod;
  modules.insert(modules_t::value_type(mod.getId(), module));

  try {
    SmartPointer<Value> exports = getFactory()->createObject();
    js::Sink sink(getFactory(), *exports);
    mod.define(sink);
    sink.close();
    module->setExports(exports);
  } CATCH_ERROR;
}


void Javascript::import(const string &id, const string &as) {
  SmartPointer<Scope> scope = impl->enterScope();

  // Find module
  auto it = modules.find(id);
  if (it == modules.end()) return THROW("Module '" << id << "' not found");

  // Get target object
  SmartPointer<Value> target = getFactory()->createObject();
  if (as.empty()) nativeProps->set(id, *target);
  else if (as == ".") target = nativeProps;
  else nativeProps->set(as, *target);

  // Copy properties
  target->copyProperties(*it->second->getExports());
}


SmartPointer<js::Value> Javascript::eval(const InputSource &source) {
  SmartFunctor<Javascript> popPath(this, &Javascript::popPath, false);

  if (!source.getName().empty() && source.getName()[0] != '<') {
    pushPath(source.getName());
    popPath.setEngaged(true);
  }

  SmartPointer<Scope> scope = impl->enterScope();
  scope->getGlobalObject()->copyProperties(*nativeProps);

  return scope->eval(source);
}


void Javascript::interrupt() {impl->interrupt();}


SmartPointer<js::StackTrace> Javascript::getStackTrace(unsigned maxFrames) {
  return impl->getStackTrace(maxFrames);
}


string Javascript::stringify(Value &value) {
  SmartPointer<Scope> scope = impl->newScope();
  SmartPointer<Value> f =
    scope->getGlobalObject()->get("JSON")->get("stringify");
  vector<SmartPointer<Value> > args;
  args.push_back(SmartPointer<Value>::Phony(&value));
  return f->call(args)->toString();
}


SmartPointer<Value> Javascript::require(const string &id) {
  auto it = modules.find(id);
  if (it != modules.end()) return it->second->getExports();

  string path = searchPath(id);
  if (path.empty()) THROW("Module '" << id << "' not found");

  // Handle package.json
  if (String::endsWith(path, "/package.json")) {
    JSON::ValuePtr package = JSON::Reader(InputSource::open(path)).parse();
    path = SystemUtilities::absolute(path, package->getString("main"));
  }

  path = SystemUtilities::getCanonicalPath(path);

  // Register module
  SmartPointer<Module> module = new Module(id, path);
  modules.insert(modules_t::value_type(id, module));

  SmartPointer<Scope> scope = impl->newScope();
  SmartPointer<Factory> factory = getFactory();
  SmartPointer<Value> exports = factory->createObject();

  if (String::endsWith(path, ".json")) {
    // Read JSON data
    js::Sink sink(factory);
    JSON::Reader::parseFile(path, sink);
    sink.close();
    exports = sink.getRoot();

  } else {
    // Push path
    SmartFunctor<Javascript> smartPopPath(this, &Javascript::popPath);
    pushPath(path);

    // Get global object for this scope
    SmartPointer<Value> global = scope->getGlobalObject();

    // Inject native properties
    global->copyProperties(*nativeProps);

    // Create module object
    SmartPointer<Value> obj = factory->createObject();
    obj->set("id", factory->create(id));
    obj->set("exports", exports);
    obj->set("filename", factory->create(path));
    obj->set("name", factory->create(id));

    // Set global vars
    global->set("id", factory->create(id));
    global->set("exports", exports);
    global->set("module", obj);

    // Set empty object for cyclic dependencies
    module->setExports(exports);

    // Compile & eval code
    scope->eval(InputSource::open(path));

    // Get exports, it may have been reassigned
    exports = obj->get("exports");
  }

  module->setExports(exports->makePersistent());

  return exports;
}


SmartPointer<Value> Javascript::require(Callback &cb, Value &args) {
  return require(args.getString(0));
}
