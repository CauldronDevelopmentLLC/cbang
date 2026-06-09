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

#pragma once

#include "Condition.h"

#include <cbang/event/AsyncSubprocess.h>
#include <cbang/event/StreamLogger.h>

#include <vector>
#include <string>


namespace cb {
  namespace Event {class Base;}

  namespace API {
    // The process behind a `cmd` condition: judged only by exit status.  Its
    // stdout is ignored (inherited); stderr is logged.
    class CmdConditionProc : public Event::AsyncSubprocess {
      Event::Base &base;
      BoolCallback cb;
      SmartPointer<Event::StreamLogger> errStr;

    public:
      CmdConditionProc(Event::Base &base, const std::vector<std::string> &cmd,
        const BoolCallback &cb) :
        Event::AsyncSubprocess(cmd), base(base), cb(cb) {}

      // From AsyncSubprocess
      void exec() override;
      void done() override;
    };
  }
}
