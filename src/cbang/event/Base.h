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

#include "EventFlag.h"

#include <cbang/SmartPointer.h>

#include <functional>

struct event_base;


namespace cb {
  namespace Event {
    class Event;

    class Base : public EventFlag {
      event_base *base;

    public:
      template <class T> struct Callback {
        typedef void (T::*member_t)(Event &, int, unsigned);
        typedef void (T::*bare_member_t)();
      };

      typedef std::function<void (Event &, int, unsigned)> callback_t;
      typedef std::function<void ()> bare_callback_t;

      Base();
      ~Base();

      struct event_base *getBase() const {return base;}

      void initPriority(int num);

      SmartPointer<Event> newEvent(callback_t cb, bool persistent = false);
      SmartPointer<Event> newEvent(int fd, unsigned events, callback_t cb);
      SmartPointer<Event> newSignal(int signal, callback_t cb,
                                    bool persistent = false);


      // Bare Callbacks
      callback_t bind(bare_callback_t cb)
      {return [cb] (Event &, int, unsigned) {cb();};}

      SmartPointer<Event> newEvent(bare_callback_t cb, bool persistent = false)
      {return newEvent(bind(cb), persistent);}

      SmartPointer<Event> newEvent(int fd, unsigned events, bare_callback_t cb)
      {return newEvent(fd, events, bind(cb));}

      SmartPointer<Event> newSignal(int signal, bare_callback_t cb,
                                    bool persistent = false)
      {return newSignal(signal, bind(cb), persistent);}


      // Member Callbacks
      template <class T>
      callback_t bind(T *obj, typename Callback<T>::member_t member) {
        using namespace std::placeholders;
        return std::bind(member, obj, _1, _2, _3);
      }

      template <class T> SmartPointer<Event> newEvent
      (T *obj, typename Callback<T>::member_t member, bool persistent = false)
      {return newEvent(bind(obj, member), persistent);}

      template <class T>
      SmartPointer<Event> newEvent(int fd, unsigned events, T *obj,
                                   typename Callback<T>::member_t member)
      {return newEvent(fd, events, bind(obj, member));}

      template <class T> SmartPointer<Event>
      newSignal(int signal, T *obj, typename Callback<T>::member_t member,
                bool persistent = false)
      {return newSignal(signal, bind(obj, member), persistent);}


      // Bare Member Callbacks
      template <class T>
      callback_t bind(T *obj, typename Callback<T>::bare_member_t member) {
        return bind(std::bind(member, obj));
      }

      template <class T>
      SmartPointer<Event> newEvent
      (T *obj, typename Callback<T>::bare_member_t member,
       bool persistent = false)
      {return newEvent(bind(obj, member), persistent);}

      template <class T>
      SmartPointer<Event> newEvent(int fd, unsigned events, T *obj,
                                   typename Callback<T>::bare_member_t member)
      {return newEvent(fd, events, bind(obj, member));}

      template <class T> SmartPointer<Event> newSignal
      (int signal, T *obj, typename Callback<T>::bare_member_t member,
       bool persistent = false)
      {return newSignal(signal, bind(obj, member), persistent);}


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
