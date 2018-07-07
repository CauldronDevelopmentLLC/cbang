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

#include "HTTPHandler.h"
#include "HTTPHandlerFactory.h"
#include "JSONHandler.h"
#include "HTTPRecastHandler.h"
#include "JSONRecastHandler.h"

#include <cbang/SmartPointer.h>

#include <vector>
#include <string>


namespace cb {
  class Resource;

  namespace Event {
    class HTTPHandlerGroup : public HTTPHandler {
      SmartPointer<HTTPHandlerFactory> factory;

      typedef std::vector<SmartPointer<HTTPHandler> > handlers_t;
      handlers_t handlers;

    public:
      HTTPHandlerGroup(const SmartPointer<HTTPHandlerFactory> &factory =
                       new HTTPHandlerFactory) : factory(factory) {}

      void addHandler(const SmartPointer<HTTPHandler> &handler);
      void addHandler(unsigned methods, const std::string &search,
                      const std::string &replace,
                      const SmartPointer<HTTPHandler> &handler);
      void addHandler(unsigned methods, const std::string &pattern,
                      const SmartPointer<HTTPHandler> &handler);

      void addHandler(const std::string &search, const std::string &replace,
                      const Resource &res);
      void addHandler(const std::string &pattern, const Resource &res);
      void addHandler(const Resource &res) {addHandler("", res);}

      void addHandler(const std::string &search, const std::string &replace,
                      const std::string &path);
      void addHandler(const std::string &pattern, const std::string &path);
      void addHandler(const std::string &path) {addHandler("", path);}

      SmartPointer<HTTPHandlerGroup>
      addGroup(unsigned methods, const std::string &search,
               const std::string &replace = std::string());

      template <class T>
      void addMember
      (T *obj, typename HTTPHandlerMemberFunctor<T>::member_t member) {
        addHandler(createHandler<T>(obj, member));
      }

      template <class T>
      void addMember
      (unsigned methods, const std::string &pattern,
       T *obj, typename HTTPHandlerMemberFunctor<T>::member_t member) {
        addHandler(methods, pattern, createHandler<T>(obj, member));
      }

      template <class T>
      void addMember
      (T *obj, typename JSONHandlerMemberFunctor<T>::member_t member) {
        addHandler(createHandler<T>(obj, member));
      }

      template <class T>
      void addMember
      (unsigned methods, const std::string &pattern,
       T *obj, typename JSONHandlerMemberFunctor<T>::member_t member) {
        addHandler(methods, pattern, createHandler<T>(obj, member));
      }

      template <class T>
      void addMember(typename HTTPRecastHandler<T>::member_t member) {
        addHandler(createHandler<T>(member));
      }

      template <class T>
      void addMember(unsigned methods, const std::string &pattern,
                     typename HTTPRecastHandler<T>::member_t member) {
        addHandler(methods, pattern, createHandler<T>(member));
      }

      template <class T>
      void addMember(typename JSONRecastHandler<T>::member_t member) {
        addHandler(createHandler<T>(member));
      }

      template <class T>
      void addMember(unsigned methods, const std::string &pattern,
                     typename JSONRecastHandler<T>::member_t member) {
        addHandler(methods, pattern, createHandler<T>(member));
      }


      template <class T> SmartPointer<HTTPHandler>
      createHandler(T *obj,
                    typename HTTPHandlerMemberFunctor<T>::member_t member) {
        return new HTTPHandlerMemberFunctor<T>(obj, member);
      }

      template <class T> SmartPointer<HTTPHandler>
      createHandler(T *obj,
                    typename JSONHandlerMemberFunctor<T>::member_t member) {
        return new JSONHandlerMemberFunctor<T>(obj, member);
      }

      template <class T> SmartPointer<HTTPHandler>
      createHandler(typename HTTPRecastHandler<T>::member_t member) {
        return new HTTPRecastHandler<T>(member);
      }

      template <class T> SmartPointer<HTTPHandler>
      createHandler(typename JSONRecastHandler<T>::member_t member) {
        return new JSONRecastHandler<T>(member);
      }


      virtual bool dispatch(Request &req) {return operator()(req);}
      virtual bool dispatch(Request &req, const JSON::ValuePtr &msg,
                            JSON::Sink &sink);

      // From HTTPHandler
      bool operator()(Request &req);
    };
  }
}
