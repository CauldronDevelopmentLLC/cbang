/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include <cbang/http/RequestHandler.h>
#include <cbang/json/Value.h>

#include <set>


namespace cb {
  namespace API {
    class APIHandler : public HTTP::RequestHandler {
      JSON::ValuePtr api;

    public:
      APIHandler(const JSON::ValuePtr &config, const JSON::ValuePtr &apiConfig);

      // From HTTP::RequestHandler
      bool operator()(HTTP::Request &req);

    protected:
      JSON::ValuePtr loadCategories(const JSON::Value &cats);
      JSON::ValuePtr loadCategory(const JSON::Value &cat);
      void loadEndpoints(const JSON::ValuePtr &endpoints,
                         const std::string &pattern,
                         const JSON::Value &config);
      JSON::ValuePtr loadMethod(const JSON::Value &method,
                                const std::set<std::string> &urlArgs,
                                const JSON::Value &endpointArgs);
    };
  }
}
