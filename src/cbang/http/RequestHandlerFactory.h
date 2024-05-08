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

#include "RequestHandler.h"

#include <functional>


namespace cb {
  namespace HTTP {
    class RequestHandlerFactory {
    public:
      typedef std::function<bool (Request &)> callback_t;
      typedef std::function<void (Request &, const JSON::ValuePtr &)>
      msg_callback_t;
      typedef std::function<void (Request &, JSON::Sink &)> sink_callback_t;
      typedef std::function<
        void (Request &, const JSON::ValuePtr &, JSON::Sink &)>
      msg_sink_callback_t;

      template <class T> struct Member {
        typedef bool (T::*member_t)(Request &);
        typedef void (T::*msg_member_t)(Request &, const JSON::ValuePtr &);
        typedef void (T::*sink_member_t)(Request &, JSON::Sink &);
        typedef void (T::*msg_sink_member_t)(
          Request &, const JSON::ValuePtr &, JSON::Sink &);

        typedef bool (T::*recast_t)();
        typedef void (T::*msg_recast_t)(const JSON::ValuePtr &);
        typedef void (T::*sink_recast_t)(JSON::Sink &);
        typedef void (T::*msg_sink_recast_t)(
          const JSON::ValuePtr &, JSON::Sink &);
      };

      static SmartPointer<RequestHandler> create(callback_t          cb);
      static SmartPointer<RequestHandler> create(msg_callback_t      cb);
      static SmartPointer<RequestHandler> create(sink_callback_t     cb);
      static SmartPointer<RequestHandler> create(msg_sink_callback_t cb);

      template <class T> static SmartPointer<RequestHandler>
      create(T *obj, typename Member<T>::member_t member) {
        return create([=] (Request &req) {return (obj->*member)(req);});
      }

      template <class T> static SmartPointer<RequestHandler>
      create(T *obj, typename Member<T>::msg_member_t member) {
        return create([=] (Request &req, const JSON::ValuePtr &msg) {
          (obj->*member)(req, msg);
        });
      }

      template <class T> static SmartPointer<RequestHandler>
      create(T *obj, typename Member<T>::sink_member_t member) {
        return create([=] (Request &req, JSON::Sink &sink) {
          (obj->*member)(req, sink);
        });
      }

      template <class T> static SmartPointer<RequestHandler>
      create(T *obj, typename Member<T>::msg_sink_member_t member) {
        return create([=] (Request &req, const JSON::ValuePtr &msg,
                           JSON::Sink &sink) {
          (obj->*member)(req, msg, sink);
        });
      }

      template <class T> static SmartPointer<RequestHandler>
      create(typename Member<T>::recast_t member) {
        return create([=] (Request &req) {return (req.cast<T>().*member)();});
      }

      template <class T> static SmartPointer<RequestHandler>
      create(typename Member<T>::msg_recast_t member) {
        return create([=] (Request &req, const JSON::ValuePtr &msg) {
          (req.cast<T>().*member)(msg);
        });
      }

      template <class T> static SmartPointer<RequestHandler>
      create(typename Member<T>::sink_recast_t member) {
        return create([=] (Request &req, JSON::Sink &sink) {
          (req.cast<T>().*member)(sink);
        });
      }

      template <class T> static SmartPointer<RequestHandler>
      create(typename Member<T>::msg_sink_recast_t member) {
        return create([=] (Request &req, const JSON::ValuePtr &msg,
                           JSON::Sink &sink) {
          (req.cast<T>().*member)(msg, sink);
        });
      }
    };
  }
}
