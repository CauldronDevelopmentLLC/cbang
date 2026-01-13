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
        min(config->getNumber("min", NAN)),
        max(config->getNumber("max", NAN)) {}

      const char *getFormat() const;

      // From ArgConstraint
      JSON::ValuePtr operator()(
        const CtxPtr &ctx, const JSON::ValuePtr &value) const override {
        T n;

        if (value->isNumber()) n = (T)value->getNumber();
        else if (value->isString())
          n = String::parse<T>(value->getString(), true);
        else CBANG_THROW("Must be a number or string");

        if (!std::isnan(min) && n < (T)min)
          CBANG_THROW("Must be greater than or equal to " << (T)min);
        if (!std::isnan(max) && (T)max < n)
          CBANG_THROW("Must be less than or equal to " << (T)max);

        if (value->isNumber()) {
          double x = value->getNumber();

          if (x < (double)std::numeric_limits<T>::lowest())
            CBANG_THROW("Less than minimum value " <<
                        (double)std::numeric_limits<T>::lowest()
                        << " for numeric type");

          if ((double)std::numeric_limits<T>::max() < x)
            CBANG_THROW("Greater than maximum value "
                        << (double)std::numeric_limits<T>::max()
                        << " for numeric type");
        }

        return value->create(n);
      }


      void addSchema(JSON::Value &schema) const override {
        schema.insert("type", "number");
        schema.insert("format", getFormat());

        if (!std::isnan(min)) schema.insert("minimum", min);
        if (!std::isnan(max)) schema.insert("maximum", max);
      }
    };


    template<>
    const char *ArgNumber<double  >::getFormat() const {return "double";}
    template<>
    const char *ArgNumber<float   >::getFormat() const {return "float";}
    template<>
    const char *ArgNumber<int64_t >::getFormat() const {return "int64";}
    template<>
    const char *ArgNumber<uint64_t>::getFormat() const {return "uint64";}
    template<>
    const char *ArgNumber<int32_t >::getFormat() const {return "int32";}
    template<>
    const char *ArgNumber<uint32_t>::getFormat() const {return "uint32";}
    template<>
    const char *ArgNumber<int16_t >::getFormat() const {return "int16";}
    template<>
    const char *ArgNumber<uint16_t>::getFormat() const {return "uint16";}
    template<>
    const char *ArgNumber<int8_t  >::getFormat() const {return "int8";}
    template<>
    const char *ArgNumber<uint8_t >::getFormat() const {return "uint8";}
  }
}
