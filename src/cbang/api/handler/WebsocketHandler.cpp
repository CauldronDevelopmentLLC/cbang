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

#include "WebsocketHandler.h"

#include <cbang/api/API.h>
#include <cbang/api/Resolver.h>
#include <cbang/api/ws/WSMapHandler.h>
#include <cbang/api/ws/WSMatchHandler.h>
#include <cbang/api/ws/WSArgsHandler.h>
#include <cbang/api/ws/WSTimeseriesHandler.h>
#include <cbang/api/ws/WSQueryHandler.h>

using namespace cb;
using namespace cb::API;


WebsocketHandler::WebsocketHandler(API &api, const JSON::ValuePtr &config) :
  api(api) {
  if (config->has("on-message")) loadHandlers(config->get("on-message"));
}


void WebsocketHandler::onMessage(const WebsocketPtr &ws,
  const JSON::ValuePtr &msg) {

  auto resolver = SmartPtr(new Resolver(api));
  resolver->set("msg", msg);

  for (auto handler: handlers)
    if (handler->onMessage(ws, resolver))
      return;

  THROW("Unhandled Websocket message " << msg->toString());
}


void WebsocketHandler::add(const WebsocketPtr &ws) {
  LOG_DEBUG(3, "Adding Websocket client with ID " << ws->getID());
  websockets[ws->getID()] = ws;
}


void WebsocketHandler::remove(const Websocket &ws) {
  LOG_DEBUG(3, "Removing Websocket client with ID " << ws.getID());
  websockets.erase(ws.getID());
}


SmartPointer<WSMessageHandler> WebsocketHandler::createHandler(
  const JSON::ValuePtr &config) {

  auto type = config->getString("handler", "");

  if (type.empty()) {
    if      (config->has("map"))        type = "map";
    else if (config->has("timeseries")) type = "timeseries";
    else if (config->has("query"))      type = "query";
    else THROW("Must specify websocket message handler type");
  }

  SmartPointer<WSMessageHandler> handler;
  if (type == "map") handler = new WSMapHandler(*this, config);
  else if (type == "timeseries")
    handler = new WSTimeseriesHandler(*this, config);
  else if (type == "query") handler = new WSQueryHandler(*this, config);
  else THROW("Unsupported websocket message handler type '" << type << "'");

  if (config->has("args"))
    handler = new WSArgsHandler(config->get("args"), handler);

  if (config->has("match"))
    handler = new WSMatchHandler(config->get("match"), handler);

  return handler;
}


void WebsocketHandler::loadHandlers(const JSON::ValuePtr &list) {
  if (!list->isList())
    THROW("Websocket `on-message` must be a list of handlers");

  for (auto config: *list)
    handlers.push_back(createHandler(config));
}


bool WebsocketHandler::operator()(HTTP::Request &req) {
  if (String::toLower(req.inFind("Upgrade")) != "websocket") return false;

  auto ws = SmartPtr(new Websocket(*this));
  ws->upgrade(req);
  add(ws);

  return true;
}
