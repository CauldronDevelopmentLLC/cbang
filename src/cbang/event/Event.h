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

#include "Base.h"

#include <cbang/SmartPointer.h>

struct event;


namespace cb {
  namespace Event {
    class EventCallback;

    class Event : SmartPointer<Event>::SelfRef, public EventFlag {
      // Only SelfRefCounter should access the SelfRef base class.
      friend class SelfRefCounter;

    public:
      typedef Base::callback_t callback_t;

    protected:
      event *e;
      callback_t cb;

    public:
      Event(Base &base, int fd, unsigned events, callback_t cb);
      Event(Base &base, int signal, callback_t cb);
      Event(event *e, callback_t cb) : e(e), cb(cb) {}
      virtual ~Event();

      event *getEvent() const {return e;}
      double getTimeout() const;

      bool isPending(unsigned events = ~0) const;
      unsigned getEvents() const;
      int getFD() const;
      void setPriority(int priority);

      void assign(Base &base, int fd, unsigned events, callback_t cb);
      void renew(int fd, unsigned events);
      void renew(int signal);
      void add(double t);
      void add();
      void readd();
      void del();
      void activate(int flags = EVENT_TIMEOUT);

      void call(int fd, short flags);

      static void enableDebugMode();
      static void enableLogging(int level = 3);
      static void enableDebugLogging();
    };

    typedef SmartPointer<Event> EventPtr;
  }
}
