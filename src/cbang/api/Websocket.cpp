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

#include "Websocket.h"
#include "Subscriber.h"
#include "Timeseries.h"

#include <cbang/api/handler/WebsocketHandler.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::API;


Websocket::~Websocket() {LOG_DEBUG(3, "~Websocket() ID " << getID());}


void Websocket::subscribe(
  Timeseries &ts, uint64_t since, unsigned maxResults) {

  auto cb = [this] (const JSON::ValuePtr &data) {
    if (!isActive()) return close(WS_STATUS_DIRTY_CLOSE, "Websocket inactive");
    send(*data);
  };

  subscriptions[&ts] = ts.subscribe(getID(), since, maxResults, cb);
}


void Websocket::unsubscribe(Timeseries &timeseries) {
  subscriptions.erase(&timeseries);
}


void Websocket::onShutdown() {
  subscriptions.clear();
  handler.remove(*this);
}


void Websocket::onMessage(const JSON::ValuePtr &msg) {
  LOG_DEBUG(3, "Websocket id=" << getID() << " msg=" << msg->toString());
  handler.onMessage(this, msg);
}
