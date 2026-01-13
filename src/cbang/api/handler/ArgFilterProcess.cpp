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

#include "ArgFilterProcess.h"

#include <cbang/Catch.h>
#include <cbang/api/Context.h>
#include <cbang/event/Base.h>
#include <cbang/event/PipeEventBuffer.h>
#include <cbang/http/Request.h>
#include <cbang/http/RequestErrorHandler.h>
#include <cbang/http/Enum.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Reader.h>

using namespace std;
using namespace cb;
using namespace Event;
using namespace cb::API;


void ArgFilterProcess::exec() {
  Subprocess::exec(cmd, Subprocess::SHELL   | Subprocess::REDIR_STDERR |
                   Subprocess::REDIR_STDOUT | Subprocess::REDIR_STDIN);

  // Make pipes non-blocking
  for (unsigned i = 0; i < 3; i++)
    getPipe(i).setBlocking(false);

  inStr  = new PipeEventBuffer(base, getPipeIn(),  Enum::EVENT_WRITE);
  outStr = new PipeEventBuffer(base, getPipeOut(), Enum::EVENT_READ);
  errStr = new StreamLogger(
    base, getPipeErr(), SSTR("PID:" << getPID() << ':'),
    CBANG_LOG_DOMAIN, CBANG_LOG_WARNING_LEVEL);

  inStr->add(ctx->getArgs()->toString());
  inStr->write();
}


void ArgFilterProcess::done() {
  try {
    outStr->read();
    errStr->flush();

    string output = outStr->toString();
    if (output.empty()) THROW("No output from arg-filter");

    auto results  = JSON::Reader::parse(output);
    unsigned code = results->getU32("code", 200);

    if (code == 200) {
      if (results->hasDict("data"))
        ctx->getArgs()->merge(results->getDict("data"));

      ctx->errorHandler([&] {
        if (!(*child)(ctx)) ctx->reply(HTTP::Status::HTTP_NOT_FOUND);
      });
      return;
    }

    if (results->hasString("error")) LOG_WARNING(results->getString("error"));
    ctx->reply((HTTP::Status::enum_t)code);
    return;

  } CATCH_ERROR;

  ctx->reply(HTTP::Enum::HTTP_INTERNAL_SERVER_ERROR);
}
