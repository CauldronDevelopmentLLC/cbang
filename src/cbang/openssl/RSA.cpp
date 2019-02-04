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

#include "RSA.h"

#include <cbang/Exception.h>

#include <openssl/rsa.h>

using namespace cb;


BigNum cb::RSA::getN() const {
  const BIGNUM *n;
  RSA_get0_key(rsa, &n, 0, 0);
  if (!n) THROW("RSA N not set");
  return BigNum(n);
}


BigNum cb::RSA::getE() const {
  const BIGNUM *e;
  RSA_get0_key(rsa, 0, &e, 0);
  if (!e) THROW("RSA E not set");
  return BigNum(e);
}


BigNum cb::RSA::getD() const {
  const BIGNUM *d;
  RSA_get0_key(rsa, 0, 0, &d);
  if (!d) THROW("RSA D not set");
  return BigNum(d);
}
