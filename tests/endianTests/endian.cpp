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

#include <cbang/net/Swab.h>
#include <cbang/String.h>
#include <cbang/Catch.h>

#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <limits>

using namespace cb;
using namespace std;


#define TEST_SWAP(SWAP, TYPE, X)                                        \
  if ((0 < x || numeric_limits<TYPE>::is_signed) &&                     \
      (int64_t)(TYPE)X == X)                                            \
    cout << X << " = "                                                  \
         << String::printf("0x%016llx", X) << " -> "                    \
         << #SWAP "(" << X << ") = "                                    \
         << String::printf("0x%016llx", (uint64_t)SWAP(X)) << " -> "    \
         << setw(10) << "(" #TYPE ")"                                   \
         << #SWAP "(" #SWAP "(" << X << ")) = "                         \
         << String::printf("0x%016llx", (uint64_t)SWAP(SWAP(X))) << " = " \
         << (TYPE)SWAP(SWAP(X)) << endl;



int main(int argc, char *argv[]) {
  try {
    if (argc != 2) {
      cout << "Usage: " << argv[0] << " <number>" << endl;
      return 1;
    }

    int64_t x = String::parseS64(argv[1]);

    TEST_SWAP(swap64, uint64_t, x);
    TEST_SWAP(swap64, uint32_t, x);
    TEST_SWAP(swap64, uint16_t, x);

    TEST_SWAP(swap64, int64_t, x);
    TEST_SWAP(swap64, int32_t, x);
    TEST_SWAP(swap64, int16_t, x);

    TEST_SWAP(swap32, uint32_t, x);
    TEST_SWAP(swap32, uint16_t, x);

    TEST_SWAP(swap32, int32_t, x);
    TEST_SWAP(swap32, int16_t, x);

    TEST_SWAP(swap16, uint16_t, x);

    TEST_SWAP(swap16, int16_t, x);

    return 0;

  } CATCH_ERROR;

  return 1;
}
