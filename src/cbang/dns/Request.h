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

#include "Type.h"
#include "Result.h"

#include <cbang/SmartPointer.h>


namespace cb {
  namespace Event {class Event;}

  namespace DNS {
    class Base;

    class Request : public RefCounted, public Error::Enum, public Type::Enum {
    protected:
      Base &base;
      std::string request;

      bool cancelled = false;
      cb::SmartPointer<Result> result;
      SmartPointer<Event::Event> timeout;

    public:
      Request(Base &base, const std::string &request);
      virtual ~Request();

      virtual Type getType() const = 0;
      const std::string &toString() const {return request;}
      bool isCancelled() const {return cancelled;}

      void cancel();
      void respond(const cb::SmartPointer<Result> &result);
      virtual void callback() = 0;

    private:
      void timedout();
    };

    typedef SmartPointer<Request> RequestPtr;
  }
}
