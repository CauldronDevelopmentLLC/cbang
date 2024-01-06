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

#include "CertificateChain.h"

#include "Certificate.h"
#include "BIStream.h"

#include <openssl/pem.h>
#include <openssl/x509_vfy.h>
#include <openssl/opensslv.h>

using namespace cb;
using namespace std;


CertificateChain::CertificateChain(const CertificateChain &o) :
  chain(X509_chain_up_ref(o.chain)) {}


CertificateChain::CertificateChain(X509_CHAIN *chain) : chain(chain) {
  if (!chain) this->chain = sk_X509_new_null();
}


CertificateChain::CertificateChain(const string &pem) :
  chain(sk_X509_new_null()) {
  parse(pem);
}


CertificateChain::~CertificateChain() {
  sk_X509_pop_free(chain, X509_free);
}


CertificateChain &CertificateChain::operator=(const CertificateChain &o) {
  sk_X509_pop_free(chain, X509_free);
  chain = X509_chain_up_ref(o.chain);
  return *this;
}


unsigned CertificateChain::size() const {return sk_X509_num(chain);}


Certificate CertificateChain::get(unsigned i) const {
  if (size() <= i) THROW("Invalid certificate chain index " << i);
  auto cert = sk_X509_value(chain, i);
  X509_up_ref(cert);
  return Certificate(cert);
}


void CertificateChain::add(const Certificate &cert) {
  X509_up_ref(cert.getX509());
  sk_X509_push(chain, cert.getX509());
}


void CertificateChain::clear() {
  sk_X509_pop_free(chain, X509_free);
  chain = sk_X509_new_null();
}


void CertificateChain::read(istream &stream) {
  BIStream bio(stream);

  X509 *cert = 0;

  while (PEM_read_bio_X509(bio.getBIO(), &cert, 0, 0)) {
    add(Certificate(cert));
    cert = 0;
  }
}


void CertificateChain::write(ostream &stream) const {
  for (unsigned i = 0; i < size(); i++)
    get(i).write(stream);
}
