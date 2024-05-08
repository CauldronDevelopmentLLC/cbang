/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include <cbang/os/Subprocess.h>
#include <cbang/time/Time.h>

#include <vector>


namespace cb {
  namespace Event {
    class AsyncSubprocess : public cb::Subprocess {
      std::vector<std::string> args;
      unsigned flags;
      int priority;
      uint64_t ts = cb::Time();

    public:
      AsyncSubprocess(
        const std::vector<std::string> &args = std::vector<std::string>(),
        unsigned flags = 0, int priority = 0);
      virtual ~AsyncSubprocess();

      const std::vector<std::string> &getArgs() const {return args;}
      std::vector<std::string> &getArgs() {return args;}
      void setArgs(const std::string &cmd);

      unsigned getFlags() const {return flags;}
      void setFlags(unsigned flags) {this->flags = flags;}

      int getPriority() const {return priority;}
      void setPriority(int priority) {this->priority = priority;}

      virtual void exec();
      virtual void done() {}

      bool operator<(const AsyncSubprocess &o) const {
        if (priority == o.priority) return ts < o.ts;
        return priority < o.priority;
      }
    };
  }
}
