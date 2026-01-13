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

#include "ArgConstraint.h"

#include <cbang/Math.h>
#include <cbang/json/Value.h>


namespace cb {
  namespace Event {class Request;}

  namespace API {
    class ArgValidator : public ArgConstraint {
      API &api;
      JSON::ValuePtr config;
      bool optional = false;
      JSON::ValuePtr defaultValue;
      std::string type;
      std::string source;
      std::vector<SmartPointer<ArgConstraint> > constraints;

    public:
      ArgValidator(API &api, const JSON::ValuePtr &config);

      bool isOptional() const {return optional;}
      bool hasDefault() const {return !defaultValue.isNull();}
      const JSON::ValuePtr &getDefault() const {return defaultValue;}
      JSON::ValuePtr getSpec() const;
      JSON::ValuePtr getSchema() const;

      bool hasSource() const {return !source.empty();}
      const std::string &getSource() const {return source;}

      void add(const SmartPointer<ArgConstraint> &constraint);

      // From ArgConstraint
      JSON::ValuePtr operator()(
        const CtxPtr &ctx, const JSON::ValuePtr &value) const override;
      void addSchema(JSON::Value &schema) const override;
    };
  }
}
