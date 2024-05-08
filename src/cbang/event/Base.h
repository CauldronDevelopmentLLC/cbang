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

#include "EventFactory.h"

#include <map>

struct event_base;


namespace cb {
  namespace DNS {class Base;}

  namespace Event {
    class Event;
    class FDPool;

    class Base : public EventFactory {
      static bool _threadsEnabled;

      bool deallocating = false;
      event_base *base;

      bool useSystemNS;
      SmartPointer<DNS::Base> dns;
      SmartPointer<FDPool> pool;

    public:
      Base(bool withThreads = false, bool useSystemNS = true,
           int priorities = -1);
      ~Base();

      bool isDeallocating() const {return deallocating;}
      struct event_base *getBase() const {return base;}

      DNS::Base &getDNS();
      FDPool &getPool();

      void initPriority(int num);
      bool hasPriorities() const {return 1 < getNumPriorities();}
      int getNumPriorities() const;
      int getNumEvents() const;
      int getNumActiveEvents() const;
      void countActiveEventsByPriority(std::map<int, unsigned> &counts) const;

      void dispatch();
      void loop();
      void loopOnce();
      bool loopNonBlock();
      void loopBreak();
      void loopContinue();
      void loopExit();

      static void enableThreads();
      static bool threadsEnabled() {return _threadsEnabled;}
    };
  }
}
