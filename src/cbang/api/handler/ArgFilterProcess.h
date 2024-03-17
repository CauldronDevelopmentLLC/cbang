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

#pragma once

#include <cbang/event/AsyncSubprocess.h>
#include <cbang/event/StreamEventBuffer.h>
#include <cbang/event/StreamLogger.h>
#include <cbang/http/RequestHandler.h>

#include <vector>


namespace cb {
  namespace Event {
    class Base;
    class Request;
  }

  namespace API {
    class ArgFilterProcess : public Event::AsyncSubprocess {
      Event::Base &base;
      SmartPointer<HTTP::RequestHandler> child;
      std::vector<std::string> cmd;
      HTTP::Request &req;

      SmartPointer<Event::StreamEventBuffer> inStr;
      SmartPointer<Event::StreamEventBuffer> outStr;
      SmartPointer<Event::StreamLogger>      errStr;

    public:
      ArgFilterProcess(
        Event::Base &base,
        SmartPointer<HTTP::RequestHandler> child,
        const std::vector<std::string> &cmd, HTTP::Request &req) :
        base(base), child(child), cmd(cmd), req(req) {}

      // From AsyncSubprocess
      void exec();
      void done();
    };
  }
}
