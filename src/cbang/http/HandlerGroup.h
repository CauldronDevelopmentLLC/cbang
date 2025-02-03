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

#include "RequestHandlerFactory.h"

#include <vector>


namespace cb {
  class Resource;

  namespace HTTP {
    class HandlerGroup : public RequestHandler {
      typedef std::vector<SmartPointer<RequestHandler> > handlers_t;
      handlers_t handlers;

      std::string prefix;
      bool autoIndex = true;

    public:
      HandlerGroup() {}
      HandlerGroup(const std::string &prefix) : prefix(prefix) {}
      virtual ~HandlerGroup() {}

      bool isEmpty() const {return handlers.empty();}

      const std::string &getPrefix() const {return prefix;}
      void setPrefix(const std::string &prefix) {this->prefix = prefix;}

      bool getAutoIndex() const {return autoIndex;}
      void setAutoIndex(bool autoIndex) {this->autoIndex = autoIndex;}

      void addHandler(const SmartPointer<RequestHandler> &handler);
      void addHandler(unsigned methods, const std::string &pattern,
                      const SmartPointer<RequestHandler> &handler);

      void addHandler(const std::string &pattern, const Resource &res);
      void addHandler(const Resource &res) {addHandler("", res);}

      void addHandler(const std::string &pattern, const std::string &path);
      void addHandler(const std::string &path) {addHandler("", path);}

      SmartPointer<HandlerGroup> addGroup();
      SmartPointer<HandlerGroup>
      addGroup(unsigned methods, const std::string &pattern,
               const std::string &prefix = std::string());

      // Member callbacks
      template <class T, typename MEMBER_T>
      void addMember(T *obj, MEMBER_T member) {
        addHandler(RequestHandlerFactory::create(obj, member));
      }

      template <class T, typename MEMBER_T>
      void addMember(unsigned methods, const std::string &pattern, T *obj,
                     MEMBER_T member) {
        addHandler(
          methods, pattern, RequestHandlerFactory::create(obj, member));
      }

      // Recast member callbacks
      template <class T, typename MEMBER_T>
      void addMember(MEMBER_T member) {
        addHandler(RequestHandlerFactory::create<T>(member));
      }

      template <class T, typename MEMBER_T>
      void addMember(unsigned methods, const std::string &pattern,
                     MEMBER_T member) {
        addHandler(methods, pattern, RequestHandlerFactory::create<T>(member));
      }

      // From RequestHandler
      bool operator()(Request &req) override;

      // Factory callbacks
      virtual SmartPointer<RequestHandler>
      createMatcher(unsigned methods, const std::string &search,
                    const SmartPointer<RequestHandler> &child);
      virtual SmartPointer<RequestHandler> createHandler(const Resource &res);
      virtual SmartPointer<RequestHandler>
      createHandler(const std::string &path);
    };
  }
}
