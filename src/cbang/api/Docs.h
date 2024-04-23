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

#include <cbang/json/Value.h>
#include <cbang/json/Dict.h>

namespace cb {
  namespace API {
    class Docs : public JSON::Dict {
      JSON::ValuePtr category;
      JSON::ValuePtr methods;
      std::set<std::string> urlArgs;
      JSON::ValuePtr endpointArgs;

    public:
      Docs(const JSON::ValuePtr &config);

      void loadCategory(const std::string &name, const JSON::ValuePtr &config);
      void loadEndpoint(const std::string &path, const JSON::ValuePtr &config);
      void loadMethod(const std::string &method, const std::string &handler,
                      const JSON::ValuePtr &config);
    };
  }
}
