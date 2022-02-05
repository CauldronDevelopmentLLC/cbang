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

#pragma once

#include "SaveOStreamConfig.h"

#include <cbang/StdTypes.h>
#include <cbang/SStream.h>

#include <iomanip>


namespace cb {
  class Hex;
  inline std::ostream &operator<<(std::ostream &stream, const Hex &h);


  /**
   * Print hexadecimal numbers.
   *
   *     cout << HEX(id, 16) << endl;
   */
  class Hex {
    uint64_t value;
    unsigned width;

  public:
    Hex(uint64_t value, unsigned width = 16) :
      value(value), width(width) {}


    std::ostream &print(std::ostream &stream) const {
      using namespace std;
      SaveOStreamConfig save(stream);
      stream << "0x" << hex << setw(width) << setfill('0') << value;
      return stream;
    }


    std::string toString() const {return SSTR(*this);}
    operator std::string () const {return toString();}
  };


  inline std::ostream &operator<<(std::ostream &stream, const Hex &h) {
    return h.print(stream);
  }
}
