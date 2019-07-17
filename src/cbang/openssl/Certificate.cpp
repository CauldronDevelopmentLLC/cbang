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

#include "Certificate.h"

#include "CertificateContext.h"
#include "SSL.h"
#include "KeyPair.h"
#include "BIStream.h"
#include "BOStream.h"

#include <cbang/Exception.h>
#include <cbang/Math.h>
#include <cbang/time/Time.h>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/opensslv.h>

#include <sstream>

using namespace cb;
using namespace std;


#if OPENSSL_VERSION_NUMBER < 0x1010000fL
#define X509_up_ref(CERT) \
  CRYPTO_add(&(CERT)->references, 1, CRYPTO_LOCK_EVP_PKEY)
#define X509_EXTENSION_get_data(e) (e)->value
#define ASN1_STRING_get0_data(e) M_ASN1_STRING_data(e)
#endif // OPENSSL_VERSION_NUMBER < 0x1010000fL


Certificate::Certificate(const Certificate &o) : cert(o.cert) {
  X509_up_ref(cert);
}


Certificate::Certificate(X509 *cert) : cert(cert) {
  SSL::init();
  if (!cert)
    if (!(this->cert = X509_new()))
      THROW("Failed to create new certificate: " << SSL::getErrorStr());
}


Certificate::Certificate(const string &pem) : cert(0) {
  SSL::init();
  if (!(this->cert = X509_new()))
    THROW("Failed to create new certificate: " << SSL::getErrorStr());
  parse(pem);
}


Certificate::~Certificate() {
  if (cert) X509_free(cert);
}


Certificate &Certificate::operator=(const Certificate &o) {
  if (cert) X509_free(cert);
  cert = o.cert;
  X509_up_ref(cert);
  return *this;
}


bool Certificate::hasPublicKey() const {return X509_get_pubkey(cert);}


void Certificate::getPublicKey(KeyPair &key) const {
  EVP_PKEY *pkey = X509_get_pubkey(cert);
  if (!pkey)
    THROW("Error getting public key from certificate: " << SSL::getErrorStr());

  if (key.getEVP_PKEY()) EVP_PKEY_free(key.getEVP_PKEY());
  key.setEVP_PKEY(pkey);
}


SmartPointer<KeyPair> Certificate::getPublicKey() const {
  SmartPointer<KeyPair> key = new KeyPair(0);
  getPublicKey(*key);
  return key;
}


void Certificate::setPublicKey(const KeyPair &key) {
  if (!X509_set_pubkey(cert, key.getEVP_PKEY()))
    THROW("Failed to set certificate's public key: " << SSL::getErrorStr());
}


void Certificate::setVersion(int version) {
  if (!X509_set_version(cert, version))
    THROW("Failed to set certificate version: " << SSL::getErrorStr());
}


int Certificate::getVersion() const {
  return X509_get_version(cert);
}


void Certificate::setSerial(long serial) {
  if (!ASN1_INTEGER_set(X509_get_serialNumber(cert), serial))
    THROW("Failed to set certificate's serial: " << SSL::getErrorStr());
}


long Certificate::getSerial() const {
  return ASN1_INTEGER_get(X509_get_serialNumber(cert));
}


void Certificate::setNotBefore(uint64_t x) {
  if (!X509_gmtime_adj(X509_get_notBefore(cert), x))
    THROW("Failed to set certificate's not before: " << SSL::getErrorStr());
}


bool Certificate::isNotBeforeInFuture() const {
  int ret = X509_cmp_current_time(X509_get_notBefore(cert));

  if (!ret)
    THROW("Failed to get certificate's not before: " << SSL::getErrorStr());

  return 0 < ret;
}


void Certificate::setNotAfter(uint64_t x) {
  if (!X509_gmtime_adj(X509_get_notAfter(cert), x))
    THROW("Failed to set certificate's not after: " << SSL::getErrorStr());
}


bool Certificate::isNotAfterInPast() const {
  int ret = X509_cmp_current_time(X509_get_notAfter(cert));

  if (!ret)
    THROW("Failed to get certificate's not after: " << SSL::getErrorStr());

  return ret < 0;
}


bool Certificate::expiredIn(unsigned secs) const {
  time_t t;
  time(&t);
  t += (time_t)secs;

  int ret = X509_cmp_time(X509_get_notAfter(cert), &t);

  if (!ret)
    THROW("Failed to get certificate's not after: " << SSL::getErrorStr());

  return ret < 0;
}


void Certificate::setIssuer(const Certificate &issuer) {
  X509_NAME *name = X509_get_subject_name(issuer.cert);
  if (!name) THROW("Failed to get issuer name: " << SSL::getErrorStr());

  if (!X509_set_issuer_name(cert, name))
    THROW("Failed to set issuer name: " << SSL::getErrorStr());
}


void Certificate::addNameEntry(const string &name, const string &value) {

  if (!X509_NAME_add_entry_by_txt(X509_get_subject_name(cert), name.c_str(),
                                  MBSTRING_ASC, (uint8_t *)value.c_str(), -1,
                                  -1, 0))
    THROW("Failed to add certificate name entry '" << name << "'='" << value
           << "': " << SSL::getErrorStr());
}


string Certificate::getNameEntry(const string &name) const {
  X509_NAME *subject = X509_get_subject_name(cert);

  if (!subject) THROW("Failed to get cetficate subject name");

  for (int i = 0; i < X509_NAME_entry_count(subject); i++) {
    X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject, i);
    ASN1_OBJECT *obj = X509_NAME_ENTRY_get_object(entry);
    int nid = OBJ_obj2nid(obj);

    if (OBJ_nid2sn(nid) == name)
      return (char *)ASN1_STRING_get0_data(X509_NAME_ENTRY_get_data(entry));
  }

  THROW("Name entry '" << name << "' not found");
}


bool Certificate::hasExtension(const string &name) const {
  return 0 <= X509_get_ext_by_NID(cert, SSL::findObject(name), -1);
}


string Certificate::getExtension(const string &name) const {
  X509_EXTENSION *ext =
    X509_get_ext(cert, X509_get_ext_by_NID(cert, SSL::findObject(name), -1));
  if (!ext) THROW("Extension '" << name << "' not in certificate");

  ostringstream stream;
  BOStream bio(stream);

  if (!X509V3_EXT_print(bio.getBIO(), ext, 0, 0))
    ASN1_STRING_print(bio.getBIO(), X509_EXTENSION_get_data(ext));

  return stream.str();
}


string Certificate::getExtension(const string &name,
                                 const string &defaultValue) const {
  return hasExtension(name) ? getExtension(name) : defaultValue;
}


bool Certificate::extensionHas(const string &name, const string &value,
                               const string &delims) {
  if (!hasExtension(name)) return false;

  vector<string> values;
  String::tokenize(getExtension(name), values);

  for (unsigned i = 0; i < values.size(); i++)
    if (values[i] == value) return true;

  return false;
}


void Certificate::addExtension(const string &name, const string &value,
                               CertificateContext *ctx) {
  X509_EXTENSION *ext =
    X509V3_EXT_conf(0, ctx ? ctx->getX509V3_CTX() : 0, (char *)name.c_str(),
                    (char *)value.c_str());
  if (!ext) THROW("Failed to create extension '" << name << "'='" << value
                  << "': " << SSL::getErrorStr());

  if (!X509_add_ext(cert, ext, -1)) {
    X509_EXTENSION_free(ext);
    THROW("Failed to add extension '" << name << "'='" << value
           << "': " << SSL::getErrorStr());
  }

  X509_EXTENSION_free(ext);
}


void Certificate::addExtensionAlias(const string &alias, const string &name) {
  if (!X509V3_EXT_add_alias(SSL::findObject(alias), SSL::findObject(name)))
    THROW("Failed to alias extension '" << alias << "' to '" << name
           << "': " << SSL::getErrorStr());
}


bool Certificate::issued(const Certificate &o) const {
  return X509_check_issued(cert, o.cert) == 0;
}


void Certificate::sign(const KeyPair &key, const string &digest) const {
  const EVP_MD *md = EVP_get_digestbyname(digest.c_str());
  if (!md) THROW("Unrecognized message digest '" << digest << "'");

  if (!X509_sign(cert, key.getEVP_PKEY(), md))
    THROW("Failed to sign Certificate: " << SSL::getErrorStr());

}


void Certificate::verify(const KeyPair &key) const {
  int ret = X509_verify(cert, key.getEVP_PKEY());
  if (!ret) THROW("Certificate verification failed");
  if (ret < 0) THROW("Certificate verification error: " << SSL::getErrorStr());
}


void Certificate::read(istream &stream) {
  BIStream bio(stream);

  if (!PEM_read_bio_X509(bio.getBIO(), &cert, 0, 0))
    THROW("Failed to read certificate: " << SSL::getErrorStr());
}


void Certificate::write(ostream &stream) const {
  BOStream bio(stream);

  if (!PEM_write_bio_X509(bio.getBIO(), cert))
    THROW("Failed to write certificate: " << SSL::getErrorStr());
}
