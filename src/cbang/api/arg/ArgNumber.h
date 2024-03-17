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

#include "ArgConstraint.h"

#include <cbang/String.h>
#include <cbang/json/Value.h>

#include <limits>


namespace cb {
  namespace API {
    template<typename T>
    class ArgNumber : public ArgConstraint {
      double min;
      double max;

    public:
      ArgNumber(const JSON::ValuePtr &config) :
        min(config->getNumber("min", NAN)), max(config->getNumber("max", NAN)) {}

      // From ArgConstraint
      void operator()(HTTP::Request &req, JSON::Value &value) const {
        T n;

        if (value.isNumber()) n = (T)value.getNumber();
        else if (value.isString())
          n = String::parse<T>(value.getString(), true);
        else CBANG_THROW("Must be a number or string");

        if (!std::isnan(min) && n < (T)min)
          CBANG_THROW("Must be greater than " << (T)min);
        if (!std::isnan(max) && (T)max < n)
          CBANG_THROW("Must be less than " << (T)max);

        if (value.isNumber()) {
          double x = value.getNumber();

          if (x < (double)std::numeric_limits<T>::min())
            CBANG_THROW("Less than minimum value " <<
                        (double)std::numeric_limits<T>::min()
                        << " for numeric type");

          if ((double)std::numeric_limits<T>::max() < x)
            CBANG_THROW("Greater than maximum value "
                        << (double)std::numeric_limits<T>::max()
                        << " for numeric type");
        }
      }
    };
  }
}
