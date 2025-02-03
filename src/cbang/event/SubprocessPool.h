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

#pragma once

#include "AsyncSubprocess.h"

#include <cbang/SmartPointer.h>

#include <queue>
#include <list>
#include <vector>
#include <cstdint>


namespace cb {
  namespace Event {
    class Base;
    class Event;

    class SubprocessPool {
      Base &base;
      unsigned maxActive;

      struct cmp {
        bool operator()(const cb::SmartPointer<AsyncSubprocess> &p1,
                        const cb::SmartPointer<AsyncSubprocess> &p2) const {
          return *p2 < *p1; // Reverse the order, lowest first
        }
      };

      typedef std::vector<cb::SmartPointer<AsyncSubprocess> > container_t;
      typedef std::priority_queue<
        cb::SmartPointer<AsyncSubprocess>, container_t, cmp> procs_t;
      procs_t procs;

      typedef std::list<cb::SmartPointer<AsyncSubprocess> > active_t;
      active_t active;

      bool quit = false;

      cb::SmartPointer<cb::Event::Event> execEvent;
      cb::SmartPointer<cb::Event::Event> signalEvent;

    public:
      SubprocessPool(Base &base);

      Base &getBase() {return base;}

      unsigned getMaxActive() const {return maxActive;}
      void setMaxActive(unsigned maxActive) {this->maxActive = maxActive;}

      unsigned getNumQueued() const {return procs.size();}
      unsigned getNumActive() const {return active.size();}

      void enqueue(const cb::SmartPointer<AsyncSubprocess> &proc);
      void shutdown();

    protected:
      void exec();
      void childSignal();
    };
  }
}
