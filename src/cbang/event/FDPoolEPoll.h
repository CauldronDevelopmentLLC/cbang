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

#pragma once

#include <cbang/config.h>

#ifdef HAVE_EPOLL

#include "FDPool.h"

#include <cbang/os/Thread.h>
#include <cbang/os/Mutex.h>

#include <list>
#include <map>
#include <algorithm>


namespace cb {
  namespace Event {
    class Base;

    class FDPoolEPoll : public FDPool, public Thread, public Mutex {
      bool destructing = false;
      int fd = -1;

      SmartPointer<Event> event;

      typedef std::map<int, SmartPointer<FD> > pool_t;
      pool_t pool;

      typedef std::pair<uint32_t, SmartPointer<FD> > remove_t;
      std::list<remove_t> removeQ;
      bool sync = false;
      uint32_t count = 0;

    public:
      FDPoolEPoll(Base &base);
      ~FDPoolEPoll();

      // From FDPool
      void add(FD &fd);
      void update(FD &fd, unsigned events);
      void remove(FD &fd);

    protected:
      void doRemove();

      // From Thread
      void run();
    };
  }
}

#endif // HAVE_EPOLL
