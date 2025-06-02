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

#include "Collection.h"

#include <vector>


namespace cb {
  namespace JSON {
    namespace Schema {
      class Schema : public Collection, public ValueType::Enum {
      protected:
        RootSchema &root;
        std::string id;
        std::vector<SmartPointer<Constraint>> constraints;

      public:
        Schema(RootSchema &root) : root(root) {}
        Schema(RootSchema &root, const JSON::Value &spec);

        const std::string &getID() const {return id;}

        // From Constraint
        bool match(const JSON::Value &v) const override;

        void parse(const JSON::Value &spec);
        ConstraintPtr parseType(
          const std::string &type, const JSON::Value &spec);
      };
    }

    using SchemaPtr = SmartPointer<Schema::Schema>;
  }
}
