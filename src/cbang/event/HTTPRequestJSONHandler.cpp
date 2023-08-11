/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "HTTPRequestJSONHandler.h"

#include <cbang/json/Writer.h>
#include <cbang/log/Logger.h>

using namespace cb::Event;
using namespace std;


bool HTTPRequestJSONHandler::operator()(Request &req) {
  try {
    // Parse JSON message
    JSON::ValuePtr msg = req.parseArgs();

    // Log JSON call
    LOG_DEBUG(5, "JSON Call: " << req.getURI().getPath() << '(' << *msg << ')');

    // Dispatch JSON call
    (*this)(req, msg);

    // Send reply
    req.reply();

  } catch (const Exception &e) {
    req.setContentType("application/json");

    if (400 <= e.getCode() && e.getCode() < 600) {
      LOG_WARNING(e.getMessages());
      req.sendJSONError((HTTPStatus::enum_t)e.getCode(), e.getMessage());

    } else {
      LOG_ERROR(e);
      req.sendError(e);
    }
  }

  return true;
}
