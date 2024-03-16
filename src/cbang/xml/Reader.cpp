/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "Reader.h"
#include "HandlerFactory.h"
#include "Adapter.h"
#include "SkipHandler.h"
#include "XIncludeHandler.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>

using namespace std;
using namespace cb;
using namespace cb::XML;


struct Reader::HandlerRecord {
  Handler *handler;
  unsigned depth;
  HandlerFactory *factory;

  HandlerRecord(Handler *handler, unsigned depth,
                HandlerFactory *factory) :
    handler(handler), depth(depth), factory(factory) {}
};


Reader::Reader(bool skipRoot) :
  skipRoot(skipRoot), depth(0), xIncludeHandler(new XIncludeHandler) {
  addFactory("include", xIncludeHandler);
}


Reader::~Reader() {delete xIncludeHandler;}


void Reader::read(const string &filename, Handler *handler) {
  pushFile(filename);
  if (handler) push(handler, 0);
  read(*SystemUtilities::open(filename, ios::in), 0);
  if (handler) pop();
  popFile();
}


void Reader::read(istream &stream, Handler *handler) {
  if (handler) push(handler, 0);

  SmartPointer<Adapter> adapter = Adapter::create();
  adapter->setFilename(getCurrentFile());
  adapter->pushHandler(this);

  SkipHandler skipper(*this);
  if (skipRoot) adapter->pushHandler(&skipper);

  adapter->read(stream);

  if (handler) pop();
}


void Reader::pushContext() {
  Processor::pushContext();
  addFactory("include", xIncludeHandler);
}


void Reader::pushFile(const std::string &filename) {
  Processor::pushFile(filename);
  if (!handlers.empty()) get().pushFile(filename);
}


void Reader::popFile() {
  Processor::popFile();
  if (!handlers.empty()) get().popFile();
}


void Reader::startElement(const string &name, const Attributes &attrs) {
  LOG_DEBUG(5, __FUNCTION__ << "(" << name << ", " << attrs.toString() << ")");

  depth++;

  HandlerFactory *factory = getFactory(name);
  if (factory) {
    push(factory->getHandler(*this, attrs), factory);
    LOG_DEBUG(5, "Reader pushed " << name << " handler");
  } else get().startElement(name, attrs);
}


void Reader::endElement(const string &name) {
  LOG_DEBUG(5, __FUNCTION__ << "(" << name << ")");

  depth--;

  if (!pop()) get().endElement(name);
  else LOG_DEBUG(5, "Reader popped " << name << " handler");
}


void Reader::text(const string &text) {
  if (depth) get().text(text);
}


void Reader::cdata(const string &data) {
  if (depth) get().cdata(data);
}


void Reader::push(Handler *handler, HandlerFactory *factory) {
  handlers.push_back(HandlerRecord(handler, depth, factory));
  get().pushFile(getCurrentFile());
}


bool Reader::pop() {
  if (!handlers.empty() && handlers.back().depth == depth + 1) {
    get().popFile();

    HandlerRecord &record = handlers.back();
    if (record.factory) record.factory->freeHandler(*this, record.handler);
    handlers.pop_back();
    return true;
  }

  return false;
}


Handler &Reader::get() {
  if (handlers.empty()) THROW("Handlers empty!");
  return *handlers.back().handler;
}
