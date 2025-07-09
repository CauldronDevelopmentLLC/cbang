/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Config.h"

#include <cbang/api/API.h>
#include <cbang/api/HandlerGroup.h>
#include <cbang/api/handler/ArgsHandler.h>
#include <cbang/api/handler/HTTPHandler.h>
#include <cbang/http/AccessHandler.h>

using namespace std;
using namespace cb;
using namespace cb::API;


Config::Config(API &api, const JSON::ValuePtr &config, const string &pattern,
  SmartPointer<Config> parent) :
  api(api), config(config), pattern(pattern), parent(parent),
  access(parent.isSet() ? parent->access : 0),
  args(parent.isSet() ? parent->args : 0) {

  if (!config->isDict()) return;

  if (config->has("args")) {
    args = args.isSet() ? new ArgDict(*args) : new ArgDict;
    args->load(api, config->get("args"));
  }

  if (config->has("allow") || config->has("deny")) {
    access = access.isSet() ?
      new HTTP::AccessHandler(*access) : new HTTP::AccessHandler;
    access->read(config);
  }
}


Config::~Config() {}


CfgPtr Config::createChild(
  const JSON::ValuePtr &config, const string &pattern) {
  return new Config(api, config, this->pattern + pattern, this);
}


HandlerPtr Config::addValidation(const HandlerPtr &_handler) const {
  HandlerPtr handler = _handler;

  if (args.isSet()) handler = new ArgsHandler(*args, handler);

  if (access.isSet()) {
    auto group = SmartPtr(new HandlerGroup);
    group->add(new HTTPHandler(access));
    group->add(handler);
    return group;
  }

  return handler;
}
