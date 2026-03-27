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

#include "Handler.h"

#include <functional>
#include <type_traits>


namespace cb {
  namespace HTTP {class Request;}

  namespace API {
    class HandlerFactory {
    public:
      using bool_cb_t = std::function<bool (const CtxPtr &)>;
      using void_cb_t = std::function<void (const CtxPtr &)>;
      using sink_cb_t = std::function<void (JSON::Sink &)>;

      using Request = HTTP::Request;
      using req_cb_t = std::function<bool (Request &)>;
      using req_msg_cb_t =
        std::function<void (Request &, const JSON::ValuePtr &)>;
      using req_sink_cb_t = std::function<void (Request &, JSON::Sink &)>;
      using req_msg_sink_cb_t =
        std::function<void (Request &, const JSON::ValuePtr &, JSON::Sink &)>;

      // API::CtrPtr callbacks
      static SmartPointer<Handler> create(bool_cb_t cb);
      static SmartPointer<Handler> create(void_cb_t cb);
      static SmartPointer<Handler> create(sink_cb_t cb);

      // Old style HTTP::Request callbacks
      static SmartPointer<Handler> create(req_cb_t cb);
      static SmartPointer<Handler> create(req_msg_cb_t cb);
      static SmartPointer<Handler> create(req_sink_cb_t cb);
      static SmartPointer<Handler> create(req_msg_sink_cb_t cb);

      // Bind class methods
      template <typename T, typename Method>
      static SmartPointer<Handler> create(T* obj, Method method) {
        return create([obj, method](auto&&... args) ->
            decltype(std::invoke(method, obj,
              std::forward<decltype(args)>(args)...)) {
          return std::invoke(
            method, obj, std::forward<decltype(args)>(args)...);
        });
      }

      // Bind class methods of subclasses of Request
      template <typename T, typename Method>
      static SmartPointer<Handler> create(Method method) {
        static_assert(std::is_base_of<Request, T>::value,
          "T must inherit from Request");

        return create([method](Request &req, auto&&... args) ->
            decltype(std::invoke(method, req.cast<T>(),
              std::forward<decltype(args)>(args)...)) {
          return std::invoke(
            method, req.cast<T>(), std::forward<decltype(args)>(args)...);
        });
      }
    };
  }
}
