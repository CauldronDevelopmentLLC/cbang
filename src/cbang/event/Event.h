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

#include "Base.h"

#include <cbang/SmartPointer.h>

struct event;


namespace cb {
  struct EF : public Event::EventFlag {}; // Make flags more accessible

  namespace Event {
    class EventCallback;

    class Event : public RefCounted, public EventFlag {
    public:
      typedef Base::callback_t callback_t;

    protected:
      event *e;
      callback_t cb;

    public:
      Event(Base &base, socket_t fdOrSignal, callback_t cb, unsigned flags);
      virtual ~Event();

      event *getEvent() const {return e;}

      callback_t getCallback() const {return cb;}
      void setCallback(callback_t cb) {this->cb = cb;}

      double getTimeout() const;

      bool isPending(unsigned events = ~0) const;
      unsigned getEvents() const;
      int getFD() const;

      void setPriority(int priority);
      int getPriority() const;

      void assign(Base &base, int fd, unsigned events, callback_t cb);
      void renew(int fd, unsigned events);
      void renew(int signal);
      void add(double t);
      void add();
      void readd();
      void next(uint64_t period);
      void del();
      void activate(int flags = EVENT_TIMEOUT);

      void call(int fd, short flags);

      static std::string getEventsString(unsigned events);

      static void enableDebugMode();
      static void enableLogging(int level = 3);
      static void enableDebugLogging();
    };

    typedef SmartPointer<Event> EventPtr;
  }
}
