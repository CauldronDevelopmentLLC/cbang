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

#define NOCRYPT // For WIN32 to avoid conflicts with openssl

#include "KeyPair.h"

#include "SSL.h"
#include "KeyContext.h"
#include "BIStream.h"
#include "BOStream.h"
#include "Digest.h"

#include <cbang/Exception.h>
#include <cbang/SmartPointer.h>
#include <cbang/log/Logger.h>
#include <cbang/Catch.h>
#include <cbang/net/Base64.h>

#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/ec.h>
#include <openssl/dh.h>
#include <openssl/engine.h>
#include <openssl/opensslv.h>

#if 0x3000000fL <= OPENSSL_VERSION_NUMBER
#include <openssl/core_names.h>
#endif

#include <cstring>
#include <sstream>
#include <algorithm> // std::min()

using namespace cb;
using namespace std;


namespace {
  int password_cb(char *buf, int size, int rwflag, void *cb) {
    try {
      string pw = (*(PasswordCallback *)cb)(rwflag, (unsigned)size);

      int length = std::min(size, (int)pw.length());
      memcpy(buf, pw.c_str(), length);
      return length;

    } CATCH_ERROR;

    return 0;
  }
}


KeyPair::KeyPair(const KeyPair &o) : key(o.key) {
  if (key) EVP_PKEY_up_ref(key);
}


KeyPair::KeyPair() {
  SSL::init();
  key = EVP_PKEY_new();
}


KeyPair::KeyPair(const string &key, mac_key_t type, ENGINE *e) {
  SSL::init();
  this->key =
    EVP_PKEY_new_mac_key(type == HMAC_KEY ? EVP_PKEY_HMAC : EVP_PKEY_CMAC,
                         e, (uint8_t *)key.c_str(), key.length());

  if (!this->key) THROW("Failed to create MAC key: " << SSL::getErrorStr());
}


KeyPair::~KeyPair() {release();}


void KeyPair::release() {
  if (key) EVP_PKEY_free(key);
  key = 0;
}


void KeyPair::setEVP_PKEY(EVP_PKEY *key) {
  release();
  this->key = key;
}


bool KeyPair::isValid() const {return EVP_PKEY_base_id(key) != EVP_PKEY_NONE;}
bool KeyPair::isRSA()   const {return EVP_PKEY_base_id(key) == EVP_PKEY_RSA;}
bool KeyPair::isDSA()   const {return EVP_PKEY_base_id(key) == EVP_PKEY_DSA;}
bool KeyPair::isDH()    const {return EVP_PKEY_base_id(key) == EVP_PKEY_DH;}
bool KeyPair::isEC()    const {return EVP_PKEY_base_id(key) == EVP_PKEY_EC;}


BigNum KeyPair::getParam(const char *id) const {
#if 0x3000000fL <= OPENSSL_VERSION_NUMBER
  BIGNUM *param = 0;
  EVP_PKEY_get_bn_param(key, id, &param);
  return BigNum(param, true);

#else
  THROW(__func__ << "() not supported with OpenSSL < v3.0");
#endif
}


BigNum KeyPair::getRSA_E() const {
  if (!isRSA()) THROW("Not an RSA key");

#if 0x3000000fL <= OPENSSL_VERSION_NUMBER
  return getParam(OSSL_PKEY_PARAM_RSA_E);

#else
  const BIGNUM *e;
  RSA_get0_key(EVP_PKEY_get0_RSA(key), 0, &e, 0);
  if (!e) THROW("RSA E not set");
  return BigNum(e);
#endif
}


BigNum KeyPair::getRSA_N() const {
  if (!isRSA()) THROW("Not an RSA key");

#if 0x3000000fL <= OPENSSL_VERSION_NUMBER
  return getParam(OSSL_PKEY_PARAM_RSA_N);

#else
  const BIGNUM *n;
  RSA_get0_key(EVP_PKEY_get0_RSA(key), &n, 0, 0);
  if (!n) THROW("RSA N not set");
  return BigNum(n);
#endif
}


unsigned KeyPair::size() const {return EVP_PKEY_size(key);}


const KeyPair &KeyPair::operator=(const KeyPair &o) {
  release();
  key = o.key;
  if (key) EVP_PKEY_up_ref(key);
  return *this;
}


bool KeyPair::operator==(const KeyPair &o) const {
#if 0x3000000fL <= OPENSSL_VERSION_NUMBER
  int ret = EVP_PKEY_eq(key, o.key);
  if (ret == -1) ret = 0;
#else
  int ret = EVP_PKEY_cmp(key, o.key);
#endif

  switch (ret) {
  case 0: return false;
  case 1: return true;
  default: THROW("Error comparing keys: " << SSL::getErrorStr());
  }
}


void KeyPair::generateRSA(unsigned bits, uint64_t pubExp,
                          SmartPointer<KeyGenCallback> callback) {
  KeyContext ctx(EVP_PKEY_RSA);
  ctx.keyGenInit();
  ctx.setRSABits(bits);
  ctx.setRSAPubExp(pubExp);
  ctx.setKeyGenCallback(callback.get());
  *this = ctx.keyGen();
}


void KeyPair::generateDSA(unsigned bits,
                          SmartPointer<KeyGenCallback> callback) {
  KeyContext ctx(EVP_PKEY_DSA);
  ctx.keyGenInit();
  ctx.setDSABits(bits);
  ctx.setKeyGenCallback(callback.get());
  *this = ctx.keyGen();
}


void KeyPair::generateDH(unsigned primeLen, int generator,
                         SmartPointer<KeyGenCallback> callback) {
  KeyContext ctx(EVP_PKEY_DH);
  ctx.keyGenInit();
  ctx.setDHPrimeLen(primeLen);
  ctx.setDHGenerator(generator);
  ctx.setKeyGenCallback(callback.get());
  *this = ctx.keyGen();
}


void KeyPair::generateEC(const string &curve,
                         SmartPointer<KeyGenCallback> callback) {
  KeyContext ctx(EVP_PKEY_DH);
  ctx.keyGenInit();
  ctx.setECCurve(curve);
  ctx.setKeyGenCallback(callback.get());
  *this = ctx.keyGen();
}


string KeyPair::toDER(bool pub) const {
  unsigned char *der = 0;
  int len = (pub ? i2d_PublicKey : i2d_PrivateKey)(key, &der);

  if (!der || len < 0)
    THROW("Failed to export " << (pub ? "public" : "private")
          << " key in DER format: " << SSL::getErrorStr());

  return string((char *)der, len);
}


string KeyPair::publicToDER () const {return toDER(true);}
string KeyPair::privateToDER() const {return toDER(false);}


void KeyPair::read(int type, bool pub, const string &s) {
  auto *p = (const unsigned char *)s.data();
  auto *key = (pub ? d2i_PublicKey : d2i_PrivateKey)(type, 0, &p, s.length());

  if (!key) THROW("Failed to read " << (pub ? "public" : "private") << " key: "
                  << SSL::getErrorStr());

  release();
  this->key = key;
}


void KeyPair::read(const string &algorithm, bool pub, const string &s) {
  read(getAlgorithmType(algorithm), pub, s);
}


void KeyPair::readPublic(const string &algorithm, const string &s) {
  read(algorithm, true, s);
}


void KeyPair::readPrivate(const string &algorithm, const string &s) {
  read(algorithm, false, s);
}


string KeyPair::publicToPEMString() const {
  ostringstream str;
  writePublicPEM(str);
  return str.str();
}


string KeyPair::privateToPEMString() const {
  ostringstream str;
  writePrivatePEM(str);
  return str.str();
}


istream &KeyPair::readPublicPEM(istream &stream) {
  BIStream bio(stream);

  if (!PEM_read_bio_PUBKEY(bio.getBIO(), &key, 0, 0))
    THROW("Failed to read public key: " << SSL::getErrorStr());

  return stream;
}


void KeyPair::readPublicPEM(const string &pem) {
  istringstream str(pem);
  readPublicPEM(str);
}


istream &KeyPair::readPrivatePEM(
  istream &stream, SmartPointer<PasswordCallback> callback) {
  BIStream bio(stream);

  if (!PEM_read_bio_PrivateKey
      (bio.getBIO(), &key, callback.isNull() ? 0 : password_cb, callback.get()))
    THROW("Failed to read private key: " << SSL::getErrorStr());

  return stream;
}


void KeyPair::readPrivatePEM(
  const string &pem, SmartPointer<PasswordCallback> callback) {
  istringstream str(pem);
  readPrivatePEM(str, callback);
}


ostream &KeyPair::writePublicPEM(ostream &stream) const {
  BOStream bio(stream);

  if (!PEM_write_bio_PUBKEY(bio.getBIO(), key))
    THROW("Failed to write public key: " << SSL::getErrorStr());

  return stream;
}


ostream &KeyPair::writePrivatePEM(ostream &stream) const {
  BOStream bio(stream);

  if (!PEM_write_bio_PrivateKey(bio.getBIO(), key, 0, 0, 0, 0, 0))
    THROW("Failed to write private key: " << SSL::getErrorStr());

  return stream;
}


string KeyPair::sign(const string &data) const {
  KeyContext ctx(*this);

  ctx.signInit();

  if (isRSA()) {
    ctx.setRSAPadding(KeyContext::PKCS1_PADDING);
    ctx.setSignatureMD("sha256");
  }

  return ctx.sign(data);
}


string KeyPair::signSHA256(const string &data) const {
  return sign(Digest::hash(data, "sha256"));
}


string KeyPair::signBase64SHA256(const string &data) const {
  return Base64().encode(signSHA256(data));
}


void KeyPair::verify(const string &signature, const string &data) const {
  KeyContext ctx(*this);
  ctx.verifyInit();
  if (isRSA()) {
    ctx.setRSAPadding(KeyContext::PKCS1_PADDING);
    ctx.setSignatureMD("sha256");
  }
  ctx.verify(signature, data);
}


void KeyPair::verifyBase64SHA256(const string &sig64,
                                 const string &data) const {
  verify(Base64().decode(sig64), Digest::hash(data, "sha256"));
}


int KeyPair::getAlgorithmType(const string &algorithm) {
  if (algorithm == "RSA")  return EVP_PKEY_RSA;
  if (algorithm == "DSA")  return EVP_PKEY_DSA;
  if (algorithm == "DH")   return EVP_PKEY_DH;
  if (algorithm == "EC")   return EVP_PKEY_EC;
  if (algorithm == "HMAC") return EVP_PKEY_HMAC;
  THROW("Unknown key algorithm '" << algorithm << "'");
}
