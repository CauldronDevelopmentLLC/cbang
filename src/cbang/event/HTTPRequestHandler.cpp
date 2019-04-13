/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "HTTPRequestHandler.h"

#include <cbang/json/Writer.h>
#include <cbang/log/Logger.h>

using namespace cb::Event;
using namespace std;


bool HTTPRequestJSONHandler::operator()(Request &req) {
  try {
    // Setup JSON output
    SmartPointer<JSON::Writer> writer = req.getJSONWriter();

    // Parse JSON message
    JSON::ValuePtr msg = req.getJSONMessage();

    // Log JSON call
    const string &path = req.getURI().getPath();
    if (msg.isNull()) LOG_DEBUG(5, "JSON Call: " << path << "()");
    else LOG_DEBUG(5, "JSON Call: " << path << '(' << *msg << ')');

    // Dispatch JSON call
    (*this)(req, msg, *writer);

    // Make sure JSON stream is complete
    writer->close();

    // Send reply
    req.reply();

  } catch (const Exception &e) {
    LOG_ERROR(e);
    req.setContentType("application/json");
    req.sendError(e);
  }

  return true;
}
