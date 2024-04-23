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

#include "Context.h"

#include <cbang/http/HandlerGroup.h>
#include <cbang/http/AccessHandler.h>
#include <cbang/api/handler/ArgsHandler.h>

using namespace std;
using namespace cb;
using namespace cb::API;


Context::Context(const JSON::ValuePtr &config, const string &pattern,
                 SmartPointer<Context> parent) :
  config(config), pattern(pattern), parent(parent) {

  if (parent.isSet()) {
    if (parent->argsHandler.isSet())
      argsHandler = new ArgsHandler(*parent->argsHandler);

    if (parent->accessHandler.isSet())
      accessHandler = new HTTP::AccessHandler(*parent->accessHandler);
  }

  if (!config->isDict()) return;

  if (config->has("args")) {
    if (argsHandler.isNull()) argsHandler = new ArgsHandler;
    argsHandler->add(config->get("args"));
  }

  if (config->has("allow") || config->has("deny")) {
    if (accessHandler.isNull()) accessHandler = new HTTP::AccessHandler;
    accessHandler->read(config);
  }
}


Context::~Context() {}


SmartPointer<Context> Context::createChild(
  const JSON::ValuePtr &config, const string &pattern) {
  return new Context(config, this->pattern + pattern, this);
}


void Context::addValidation(HTTP::HandlerGroup &group) {
  if (accessHandler.isSet()) group.addHandler(accessHandler);
  if (argsHandler.isSet())   group.addHandler(argsHandler);
}
