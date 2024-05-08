/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "KeyCert.h"

#include <cbang/String.h>
#include <cbang/openssl/Certificate.h>
#include <cbang/openssl/CSR.h>

using namespace cb;
using namespace cb::ACMEv2;
using namespace std;


KeyCert::KeyCert(const string &domains, const KeyPair &key,
                 const CertificateChain &chain) :
  key(key), chain(chain) {
  String::tokenize(domains, this->domains);
}


bool KeyCert::expiredIn(unsigned secs) const {
  // Expired if certificate empty, expired or does not match key
  if (!hasCert() || chain.get(0).expiredIn(secs) ||
      chain.get(0).getPublicKey() != key) return true;

  // Check cert applies to all domains
  auto cert = chain.get(0);
  for (unsigned i = 0; i < domains.size(); i++)
    if (!cert.checkHost(domains[i])) return true;

  return false;
}


SmartPointer<CSR> KeyCert::makeCSR() const {
  if (domains.empty()) THROW("No domains set");

  SmartPointer<CSR> csr = new CSR;

  csr->addNameEntry("CN", domains[0]);
  csr->addExtension("subjectAltName", "DNS:" + String::join(domains, ", DNS:"));
  csr->sign(key, "sha256");

  return csr;
}
