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

#include "JSONWebsocket.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/io/VectorStream.h>


using namespace cb;
using namespace cb::WS;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "WS" << getID() << ':'


void JSONWebsocket::send(function<void (JSON::Sink &sink)> cb) {
  VectorStream<> stream(msgBuf);
  JSON::Writer writer(stream, 0, true);

  cb(writer);

  writer.close();
  stream.flush();
  Websocket::send(msgBuf.data(), msgBuf.size());
  msgBuf.clear();
}


void JSONWebsocket::send(const JSON::Value &value) {
  LOG_DEBUG(6, "Sending: " << value);
  send([&value] (JSON::Sink &sink) {value.write(sink);});
}


void JSONWebsocket::onMessage(const char *data, uint64_t length) {
  auto value = JSON::Reader::parse(InputSource(data, length));
  LOG_DEBUG(6, "Received: " << *value);
  onMessage(value);
}
