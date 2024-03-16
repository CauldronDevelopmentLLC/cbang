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

#include <limits>
#include <cmath>
#include <cfloat>

#if defined(_WIN32) && defined(max)
#undef max
#endif
#if defined(_WIN32) && defined(min)
#undef min
#endif


namespace cb {
  namespace Math {
    static const double PI = 3.14159265358979323846;

    inline static double nextUp(double x) {
      return std::nextafter(x, std::numeric_limits<double>::infinity());
    }


    inline static double nextDown(double x) {
      return std::nextafter(x, -std::numeric_limits<double>::infinity());
    }
  }
}
