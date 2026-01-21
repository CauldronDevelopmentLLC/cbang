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

#include "Context.h"

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::API;


Context::Context(API &api, HTTP::Request &req) :
  api(api), req(req), args(new JSON::Dict), resolver(new Resolver(api, req)) {}


Context::Context(API &api, const WebsocketPtr &ws) :
  Context(api, ws->getRequest()) {this->ws = ws;}


void Context::setSession(const SmartPointer<HTTP::Session> &session) {
  req.setSession(session);
  resolver->setSession(session);
}


void Context::reply(HTTP::Status code, const JSON::ValuePtr &msg) const {
  reply(code, [&] (JSON::Sink &sink) {msg->write(sink);});
}


void Context::reply(const JSON::ValuePtr &msg) const {reply(HTTP_OK, msg);}


void Context::reply(
  HTTP::Status code, function<void (JSON::Sink &sink)> cb) const {

  if (ws.isSet()) {
    auto ref = resolver->select("msg.$ref");

    if (ref.isNull()) ws->send(cb);
    else ws->send([&] (JSON::Sink &sink) {
      sink.beginDict();
      sink.insert("$ref", *ref);
      sink.beginInsert("data");
      cb(sink);
      sink.endDict();
    });

  } else req.reply(code, cb);
}


void Context::reply(function<void (JSON::Sink &sink)> cb) const {
  reply(HTTP_OK, cb);
}


void Context::reply(HTTP::Status code, const string &text) const {
  string msg = text.empty() ? code.toString() : text;

  if (ws.isNull()) req.reply(code, msg);

  else reply([&] (JSON::Sink &sink) {
    sink.beginDict();
    sink.insert("error", code);
    sink.insert("message", msg);
    sink.endDict();
  });
}


void Context::reply(const std::exception &e) const {
  LOG_ERROR(e.what());
  reply(HTTP_INTERNAL_SERVER_ERROR, e.what());
}


void Context::reply(const Exception &e) const {
  if (400 <= e.getCode() && e.getCode() < 600)
    LOG_WARNING("REQ" << req.getID() << ':' << req.getClientAddr() << ':'
      << e.getMessages());

  else {
    if (!CBANG_LOG_DEBUG_ENABLED(3)) LOG_WARNING(e.getMessages());
    LOG_DEBUG(3, e);
  }

  reply([&] (JSON::Sink &sink) {e.write(sink);});
}


void Context::errorHandler(function<void ()> cb) const {
  try {cb();}
  catch (Exception &e) {reply(e);}
  catch (exception &e) {reply(e);}
}
