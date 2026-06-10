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

#include <cbang/event/AsyncSubprocess.h>
#include <cbang/event/StreamEventBuffer.h>
#include <cbang/event/StreamLogger.h>
#include <cbang/api/Handler.h>
#include <cbang/os/TemporaryDirectory.h>

#include <vector>
#include <string>


namespace cb {
  namespace Event {class Base;}

  namespace API {
    class ExecProcess : public Event::AsyncSubprocess {
      Event::Base &base;
      std::vector<std::string> cmd;
      std::string input;
      SmartPointer<TemporaryDirectory> tmpDir;
      const CtxPtr ctx;
      const Cont next;

      SmartPointer<Event::StreamEventBuffer> inStr;
      SmartPointer<Event::StreamEventBuffer> outStr;
      SmartPointer<Event::StreamLogger>      errStr;

    public:
      ExecProcess(
        Event::Base &base, const std::vector<std::string> &cmd,
        const std::string &input,
        const SmartPointer<TemporaryDirectory> &tmpDir, const CtxPtr &ctx,
        const Cont &next) :
        base(base), cmd(cmd), input(input), tmpDir(tmpDir), ctx(ctx),
        next(next) {}

      // From AsyncSubprocess
      void exec() override;
      void done() override;
    };
  }
}
