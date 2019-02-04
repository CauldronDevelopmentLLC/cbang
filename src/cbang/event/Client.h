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

#include "PendingRequest.h"

#include <cbang/SmartPointer.h>

#include <map>
#include <functional>


namespace cb {
  class URI;
  class SSLContext;

  namespace Event {
    class Base;
    class DNSBase;

    class Client {
      Base &base;
      DNSBase &dns;
      SmartPointer<SSLContext> sslCtx;
      int priority;

    public:
      template <class T> struct Callback {
        typedef void (T::*member_t)(Request *, int);
      };

      typedef PendingRequest::callback_t callback_t;

      Client(Base &base, DNSBase &dns);
      Client(Base &base, DNSBase &dns, const SmartPointer<SSLContext> &sslCtx);
      ~Client();

      Base &getBase() {return base;}
      DNSBase &getDNS() {return dns;}
      const cb::SmartPointer<SSLContext> &getSSLContext() const {return sslCtx;}

      int getPriority() const {return priority;}
      void setPriority(int priority) {this->priority = priority;}

      SmartPointer<PendingRequest>
      call(const URI &uri, unsigned method, const char *data, unsigned length,
           callback_t cb);

      SmartPointer<PendingRequest>
      call(const URI &uri, unsigned method, const std::string &data,
           callback_t cb);

      SmartPointer<PendingRequest>
      call(const URI &uri, unsigned method, callback_t cb);


      // Member callbacks
      template <class T>
      callback_t bind(T *obj, typename Callback<T>::member_t member) {
        using namespace std::placeholders;
        return std::bind(member, obj, _1, _2);
      }

      template <class T> SmartPointer<PendingRequest>
      call(const URI &uri, unsigned method, const char *data, unsigned length,
           T *obj, typename Callback<T>::member_t member)
      {return call(uri, method, data, length, bind(obj, member));}

      template <class T> SmartPointer<PendingRequest>
      call(const URI &uri, unsigned method, const std::string &data,
           T *obj, typename Callback<T>::member_t member)
      {return call(uri, method, data, bind(obj, member));}

      template <class T> SmartPointer<PendingRequest>
      call(const URI &uri, unsigned method,
           T *obj, typename Callback<T>::member_t member)
      {return call(uri, method, bind(obj, member));}
    };
  }
}
