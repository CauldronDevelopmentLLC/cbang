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

#include "HTTPRequestErrorHandler.h"
#include "Request.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>

using namespace cb::Event;


bool HTTPRequestErrorHandler::operator()(Request &req) {
  try {
    if (!child(req)) req.sendError(HTTPStatus::HTTP_NOT_FOUND);

  } catch (cb::Exception &e) {
    if (400 <= e.getCode() && e.getCode() < 600) {
      LOG_WARNING("REQ" << req.getID() << ':' << req.getClientIP() << ':'
                  << e.getMessages());
      req.reply((HTTPStatus::enum_t)e.getCode());

    } else {
      if (!CBANG_LOG_DEBUG_ENABLED(3)) LOG_WARNING(e.getMessages());
      LOG_DEBUG(3, e);
      req.sendError(e);
    }

  } catch (std::exception &e) {
    LOG_ERROR(e.what());
    req.sendError(e);

  } catch (...) {
    LOG_ERROR(HTTPStatus(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR)
              .getDescription());
    req.sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
  }

  return true;
}
