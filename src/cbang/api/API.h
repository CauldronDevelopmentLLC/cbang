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

#include "Context.h"

#include <cbang/http/Client.h>
#include <cbang/http/HandlerGroup.h>

#include <cbang/event/Base.h>
#include <cbang/event/SubprocessPool.h>

#include <cbang/oauth2/Providers.h>
#include <cbang/json/Value.h>
#include <cbang/http/SessionManager.h>
#include <cbang/db/maria/Connector.h>

#include <functional>


namespace cb {
  class Options;

  namespace API {
    class API : public HTTP::HandlerGroup {
      Options &options;
      JSON::ValuePtr config;

      JSON::ValuePtr spec;
      std::string category;
      bool hideCategory = false;

      SmartPointer<HTTP::Client>          client;
      SmartPointer<OAuth2::Providers>     oauth2Providers;
      SmartPointer<HTTP::SessionManager>  sessionManager;
      SmartPointer<MariaDB::Connector>    connector;
      SmartPointer<Event::SubprocessPool> procPool;

      typedef HTTP::RequestHandlerPtr RequestHandlerPtr;
      typedef std::map<std::string, RequestHandlerPtr> callbacks_t;
      callbacks_t callbacks;

    public:
      API(Options &options);
      ~API();

      Options &getOptions() const {return options;}
      const JSON::ValuePtr &getConfig() const {return config;}
      JSON::ValuePtr getSpec() const {return spec;}

      void setClient(const SmartPointer<HTTP::Client> &x) {client = x;}
      void setOAuth2Providers(const SmartPointer<OAuth2::Providers> &x)
        {oauth2Providers = x;}
      void setSessionManager(const SmartPointer<HTTP::SessionManager> &x)
        {sessionManager = x;}
      void setDBConnector(const SmartPointer<MariaDB::Connector> &x)
        {connector = x;}
      void setProcPool(const SmartPointer<Event::SubprocessPool> &x)
        {procPool = x;}

      HTTP::Client          &getClient()          {return *client;}
      OAuth2::Providers     &getOAuth2Providers() {return *oauth2Providers;}
      HTTP::SessionManager  &getSessionManager()  {return *sessionManager;}
      MariaDB::Connector    &getDBConnector()     {return *connector;}
      Event::SubprocessPool &getProcPool()        {return *procPool;}

      void load(const JSON::ValuePtr &config);

      void bind(const std::string &key, const RequestHandlerPtr &handler);

      template <class T, typename MEMBER_T>
      void bind(const std::string &key, T *obj, MEMBER_T member) {
        bind(key, HTTP::RequestHandlerFactory::create(obj, member));
      }

      template <class T, typename MEMBER_T>
      void bind(const std::string &key, MEMBER_T member) {
        bind(key, HTTP::RequestHandlerFactory::create<T>(member));
      }

    protected:
      typedef SmartPointer<Context> CtxPtr;

      virtual std::string getEndpointType(const JSON::ValuePtr &config) const;
      virtual JSON::ValuePtr getEndpointTypes(
        const JSON::ValuePtr &config) const;
      virtual RequestHandlerPtr createEndpointHandler(
        const JSON::ValuePtr &types, const JSON::ValuePtr &config);
      virtual RequestHandlerPtr createMethodsHandler(
        const std::string &methods, const CtxPtr &ctx);
      virtual RequestHandlerPtr createAPIHandler(const CtxPtr &ctx);

      virtual void addTagSpec(
        const std::string &tag, const JSON::ValuePtr &config);
      virtual void addToSpec(const std::string &methods, const CtxPtr &ctx);
    };
  }
}
