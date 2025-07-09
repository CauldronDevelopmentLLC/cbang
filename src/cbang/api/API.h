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

#include "Config.h"
#include "HandlerGroup.h"

#include <cbang/http/Client.h>
#include <cbang/http/HandlerGroup.h>

#include <cbang/event/Base.h>
#include <cbang/event/SubprocessPool.h>

#include <cbang/oauth2/Providers.h>
#include <cbang/json/Value.h>
#include <cbang/http/SessionManager.h>
#include <cbang/db/EventLevelDB.h>
#include <cbang/db/maria/Connector.h>

#include <functional>


namespace cb {
  class Options;

  namespace API {
    class API : public HTTP::RequestHandler, public HandlerGroup {
      Options       &options;
      JSON::ValuePtr config;

      JSON::ValuePtr spec;
      std::string    category;
      bool           hideCategory = false;

      SmartPointer<HTTP::Client>          client;
      SmartPointer<OAuth2::Providers>     oauth2Providers;
      SmartPointer<HTTP::SessionManager>  sessionManager;
      SmartPointer<MariaDB::Connector>    connector;
      SmartPointer<Event::SubprocessPool> procPool;
      SmartPointer<EventLevelDB>          timeseriesDB;
      JSON::ValuePtr                      optionValues;

      using RequestHandlerPtr = HTTP::RequestHandlerPtr;
      std::map<std::string, RequestHandlerPtr> callbacks;
      std::map<std::string, JSON::ValuePtr>    args;
      std::map<std::string, HandlerPtr>        queries;
      std::map<std::string, HandlerPtr>        timeseries;

    public:
      API(Options &options);
      ~API();

      const JSON::ValuePtr &getOptions()  const {return optionValues;}
      const JSON::ValuePtr &getConfig()   const {return config;}
      JSON::ValuePtr        getSpec()     const {return spec;}
      const std::string    &getCategory() const {return category;}

      void setClient(const SmartPointer<HTTP::Client> &x) {client = x;}
      void setOAuth2Providers(const SmartPointer<OAuth2::Providers> &x)
        {oauth2Providers = x;}
      void setSessionManager(const SmartPointer<HTTP::SessionManager> &x)
        {sessionManager = x;}
      void setDBConnector(const SmartPointer<MariaDB::Connector> &x)
        {connector = x;}
      void setProcPool(const SmartPointer<Event::SubprocessPool> &x)
        {procPool = x;}
      void setTimeseriesDB(const SmartPointer<EventLevelDB> &x)
        {timeseriesDB = x;}

      HTTP::Client          &getClient()          {return *client;}
      OAuth2::Providers     &getOAuth2Providers() {return *oauth2Providers;}
      HTTP::SessionManager  &getSessionManager()  {return *sessionManager;}
      MariaDB::Connector    &getDBConnector()     {return *connector;}
      Event::SubprocessPool &getProcPool()        {return *procPool;}
      EventLevelDB          &getTimeseriesDB()    {return *timeseriesDB;}

      void load(const JSON::ValuePtr &config);

      void bind(const std::string &key, const RequestHandlerPtr &handler);

      template <class T, typename METHOD_T>
      void bind(const std::string &key, T *obj, METHOD_T method) {
        bind(key, HTTP::RequestHandlerFactory::create(obj, method));
      }

      template <class T, typename METHOD_T>
      void bind(const std::string &key, METHOD_T method) {
        bind(key, HTTP::RequestHandlerFactory::create<T>(method));
      }

      static std::string resolve(const std::string &category,
        const std::string &name);
      std::string resolve(const std::string &name) const;

      void addArgs(const std::string &name, const JSON::ValuePtr &args);
      const JSON::ValuePtr &getArgs(const std::string &name) const;

      void addQuery(const std::string &name, const HandlerPtr &handler);
      HandlerPtr getQuery(
        const std::string &category, const std::string &name) const;
      HandlerPtr getQuery(const std::string &name) const;
      HandlerPtr getQuery(const JSON::ValuePtr &config);

      void addTimeseries(const std::string &name, const HandlerPtr &handler);
      HandlerPtr getTimeseries(
        const std::string &category, const std::string &name) const;
      HandlerPtr getTimeseries(const std::string &name) const;

      virtual std::string getEndpointType(const JSON::ValuePtr &config) const;
      virtual JSON::ValuePtr getEndpointTypes(
        const JSON::ValuePtr &config) const;
      virtual HandlerPtr createEndpointHandler(
        const JSON::ValuePtr &types, const CfgPtr &cfg);
      virtual HandlerPtr wrapEndpoint(
        const HandlerPtr &handler, const CfgPtr &cfg);
      virtual HandlerPtr createMethodsHandler(
        const std::string &methods, const CfgPtr &cfg);
      virtual HandlerPtr createAPIHandler(const CfgPtr &cfg);

      virtual void addTagSpec(
        const std::string &tag, const JSON::ValuePtr &config);
      virtual void addToSpec(const std::string &methods, const CfgPtr &cfg);

      // From HTTP::RequestHandler
      bool operator()(HTTP::Request &req) override;

      using HandlerGroup::operator();
    };
  }
}
