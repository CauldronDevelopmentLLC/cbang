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

#include "HandlerFactory.h"

#include <cbang/api/handler/FunctionHandler.h>

using namespace std;
using namespace cb;
using namespace cb::API;


SmartPointer<Handler> HandlerFactory::create(bool_cb_t cb) {
  return new FunctionHandler(cb);
}


SmartPointer<Handler> HandlerFactory::create(void_cb_t cb) {
  return create((bool_cb_t)[cb] (const CtxPtr &ctx) {cb(ctx); return true;});
}


SmartPointer<Handler> HandlerFactory::create(sink_cb_t cb) {
  return create((bool_cb_t)[cb] (const CtxPtr &ctx) {
    ctx->reply(cb);
    return true;
  });
}


SmartPointer<Handler> HandlerFactory::create(req_cb_t cb) {
  return create((bool_cb_t)[cb] (const CtxPtr &ctx) {
    return cb(ctx->getRequest());
  });
}


SmartPointer<Handler> HandlerFactory::create(req_msg_cb_t cb) {
  return create((bool_cb_t)[cb] (const CtxPtr &ctx) {
    auto &req = ctx->getRequest();
    auto msg  = req.getJSONMessage();
    if (msg.isNull()) msg = req.parseArgs();
    cb(ctx->getRequest(), msg);
    return true;
  });
}


SmartPointer<Handler> HandlerFactory::create(req_sink_cb_t cb) {
  return create((bool_cb_t)[cb] (const CtxPtr &ctx) {
    ctx->reply([cb, ctx] (JSON::Sink &sink) {cb(ctx->getRequest(), sink);});
    return true;
  });
}


SmartPointer<Handler> HandlerFactory::create(req_msg_sink_cb_t cb) {
  return create((bool_cb_t)[cb] (const CtxPtr &ctx) {
    auto &req = ctx->getRequest();
    auto msg  = req.getJSONMessage();
    if (msg.isNull()) msg = req.parseArgs();
    ctx->reply([cb, &req, msg] (JSON::Sink &sink) {cb(req, msg, sink);});
    return true;
  });
}
