/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "Arguments.h"

#include "Handler.h"

#include <cbang/Exception.h>

using namespace std;
using namespace cb::Script;


bool Arguments::check(unsigned min, unsigned max) const {
  if (max == (unsigned)~0) max = min;
  return min <= size() && size() <= max;
}


void Arguments::parse(Arguments &args, const std::string &s) {
  Handler::parse(args, s);
}


void Arguments::invalidNum() const {
  THROWS("Invalid number of arguments " << size() - 1 << " for function '"
         << (*this)[0] << "'");
}


void Arguments::invalid(unsigned index) const {
  THROWS("Invalid argument " << index - 1 << " '" << (*this)[index]
         << "' for function '" << (*this)[0] << "'");
}
