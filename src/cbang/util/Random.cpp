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

#include "Random.h"

#include <cbang/config.h>
#include <cbang/Exception.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/SSL.h>

#include <openssl/rand.h>
#else
#include <cstdlib>
#endif

using namespace cb;
using namespace std;


Random::Random(Inaccessible) {}


void Random::addEntropy(const void *buffer, uint32_t bytes, double entropy) {
#ifdef HAVE_OPENSSL
  RAND_add(buffer, bytes, entropy ? entropy : bytes);

#else
  if (sizeof(unsigned) <= bytes) srand(*(unsigned *)buffer);
#endif
}


void Random::bytes(void *buffer, uint32_t bytes) {
#ifdef HAVE_OPENSSL
  if (RAND_bytes((unsigned char *)buffer, bytes) != 1)
    THROW("Failed to get random bytes: " << SSL::getErrorStr());

#else
  while (1 < bytes) {
    bytes -= 2;
    ((uint16_t *)buffer)[bytes / 2] = ::rand();
  }

  if (bytes) *(uint8_t *)buffer = ::rand();
#endif
}


string Random::string(unsigned length) {
  SmartPointer<char>::Array buffer = new char[length];
  bytes(buffer.get(), length);
  return std::string(buffer.get(), length);
}
