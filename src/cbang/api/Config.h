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

#include "Handler.h"

#include <cbang/json/Value.h>

namespace cb {
  namespace HTTP {class AccessHandler;}

  namespace API {
    class API;
    class ArgDict;

    class Config : public RefCounted {
      API &api;
      JSON::ValuePtr config;
      std::string pattern;
      SmartPointer<Config> parent;

      SmartPointer<HTTP::AccessHandler> access;
      SmartPointer<ArgDict> args;

    public:
      Config(API &api, const JSON::ValuePtr &config,
        const std::string &pattern = "", SmartPointer<Config> parent = 0);
      virtual ~Config();

      const std::string &getPattern() const {return pattern;}
      const JSON::ValuePtr &getConfig() const {return config;}

      const SmartPointer<HTTP::AccessHandler> &getAccessHandler() const
        {return access;}
      const SmartPointer<ArgDict> &getArgs() const {return args;}

      SmartPointer<Config> createChild(
        const JSON::ValuePtr &config, const std::string &pattern = "");
      HandlerPtr addValidation(const HandlerPtr &handler) const;
    };

    using CfgPtr = SmartPointer<Config>;
  }
}
