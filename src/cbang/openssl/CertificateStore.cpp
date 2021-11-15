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

#include "CertificateStore.h"

#include "SSL.h"
#include "Certificate.h"
#include "CertificateStoreContext.h"
#include "CertificateChain.h"
#include "CRL.h"

#include <openssl/x509_vfy.h>
#include <openssl/opensslv.h>

using namespace cb;

#if OPENSSL_VERSION_NUMBER < 0x1010000fL
#define X509_STORE_up_ref(STORE)                            \
  CRYPTO_add(&(STORE)->references, 1, CRYPTO_LOCK_EVP_PKEY)
#define X509_EXTENSION_get_data(e) (e)->value
#endif /* OPENSSL_VERSION_NUMBER < 0x1010000fL */


CertificateStore::CertificateStore(const CertificateStore &o) : store(o.store) {
  X509_STORE_up_ref(store);
}


CertificateStore::CertificateStore(X509_STORE *store) : store(store) {
  SSL::init();

  if (store) X509_STORE_up_ref(store);
  else if (!(this->store = X509_STORE_new()))
    THROW("Failed to create new certificate store: " << SSL::getErrorStr());
}


CertificateStore::~CertificateStore() {if (store) X509_STORE_free(store);}


CertificateStore &CertificateStore::operator=(const CertificateStore &o) {
  if (store) X509_STORE_free(store);
  store = o.store;
  X509_STORE_up_ref(store);
  return *this;
}


void CertificateStore::add(const Certificate &cert) {
  if (!X509_STORE_add_cert(store, cert.getX509()))
    THROW("Failed to add certificate to store: " << SSL::getErrorStr());
}


void CertificateStore::add(const CRL &crl) {
  if (!X509_STORE_add_crl(store, crl.getX509_CRL()))
    THROW("Failed to add CRL to store: " << SSL::getErrorStr());
}


void CertificateStore::verify(const Certificate &cert) const {
  CertificateStoreContext(*this, cert).verify();
}


void CertificateStore::verify(const Certificate &cert,
                              const Certificate &inter) const {
  CertificateChain chain;
  chain.add(inter);
  CertificateStoreContext(*this, cert, chain).verify();
}
