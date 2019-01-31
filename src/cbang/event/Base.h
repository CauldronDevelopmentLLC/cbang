/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2017, Cauldron Development LLC
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

#include "EventCallback.h"
#include "BareEventCallback.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/CallbackMethods.h>

struct event_base;


namespace cb {
  namespace Event {
    class Event;

    class Base : public EventFlag {
      event_base *base;

    public:
      Base();
      ~Base();

      struct event_base *getBase() const {return base;}

      void initPriority(int num);

      SmartPointer<Event>
      newPersistentEvent(const SmartPointer<EventCallback> &cb);
      SmartPointer<Event> newEvent(const SmartPointer<EventCallback> &cb);
      SmartPointer<Event> newEvent(int fd, unsigned events,
                                   const SmartPointer<EventCallback> &cb);
      SmartPointer<Event>
      newPersistentSignal(int signal, const SmartPointer<EventCallback> &cb);
      SmartPointer<Event>
      newSignal(int signal, const SmartPointer<EventCallback> &cb);

      CBANG_CALLBACK_METHODS(Event, SmartPointer<Event>, newPersistentEvent);
      CBANG_CALLBACK_METHODS                                    \
      (BareEvent, SmartPointer<Event>, newPersistentEvent);
      CBANG_CALLBACK_METHODS(Event, SmartPointer<Event>, newEvent);
      CBANG_CALLBACK_METHODS(BareEvent, SmartPointer<Event>, newEvent);
      CBANG_CALLBACK_METHODS                                   \
      (Event, SmartPointer<Event>, newEvent, int, unsigned);
      CBANG_CALLBACK_METHODS                                           \
      (BareEvent, SmartPointer<Event>, newEvent, int, unsigned);
      CBANG_CALLBACK_METHODS                                   \
      (Event, SmartPointer<Event>, newPersistentSignal, int);
      CBANG_CALLBACK_METHODS                                           \
      (BareEvent, SmartPointer<Event>, newPersistentSignal, int);
      CBANG_CALLBACK_METHODS(Event, SmartPointer<Event>, newSignal, int);
      CBANG_CALLBACK_METHODS(BareEvent, SmartPointer<Event>, newSignal, int);

      void dispatch();
      void loop();
      void loopOnce();
      bool loopNonBlock();
      void loopBreak();
      void loopContinue();
      void loopExit();
    };
  }
}
