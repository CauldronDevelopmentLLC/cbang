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

#include "Number.h"

#include <cmath>

using namespace std;
using namespace cb;
using namespace cb::JSON::Schema;

namespace {
  double getNum(
    const JSON::Value &spec, const string &name, double defaultVal) {
    auto it = spec.find(name);
    return it ? (*it)->getNumber() : defaultVal;
  }
}


Number::Number(const JSON::Value &spec) :
  integer(spec.getString("type") == "integer"),
  multipleOf  (getNum(spec, "multipleOf", numeric_limits<double>::quiet_NaN())),
  minimum     (getNum(spec, "minimum",          numeric_limits<double>::min())),
  exclusiveMin(getNum(spec, "exclusiveMinimum", numeric_limits<double>::min())),
  maximum     (getNum(spec, "maximum",          numeric_limits<double>::max())),
  exclusiveMax(getNum(spec, "exclusiveMaximum", numeric_limits<double>::max()))
  {}


bool Number::match(const JSON::Value &value) const {
  if (!value.isNumber()) return false;

  double x = value.getNumber();
  if (integer && trunc(x) != x) return false;

  if (isfinite(multipleOf) && fmod(x, multipleOf)) return false;

  if (x < minimum || x <= exclusiveMin || maximum < x || exclusiveMax <= x)
    return false;

  return true;
}
