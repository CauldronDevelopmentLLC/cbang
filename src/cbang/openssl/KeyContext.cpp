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

#include "KeyContext.h"

#include "SSL.h"
#include "KeyPair.h"
#include "Digest.h"
#include "KeyGenCallback.h"
#include "BigNum.h"

#include <cbang/Exception.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/dh.h>
#include <openssl/ec.h>

using namespace std;
using namespace cb;


KeyContext::KeyContext(int nid, ENGINE *e) : ctx(0), deallocate(true) {
  if (!(ctx = EVP_PKEY_CTX_new_id(nid, e)))
    THROW("Failed to create key context: " << SSL::getErrorStr());
}


KeyContext::KeyContext(const string &algorithm, ENGINE *e) :
  ctx(0), deallocate(true) {
  if (!(ctx = EVP_PKEY_CTX_new_id(SSL::findObject(algorithm), e)))
    THROW("Failed to create key context: " << SSL::getErrorStr());
}


KeyContext::KeyContext(const KeyPair &keyPair) :
  ctx(EVP_PKEY_CTX_new(keyPair.getEVP_PKEY(), 0)), deallocate(true) {}


KeyContext::KeyContext(EVP_PKEY *pkey, ENGINE *e) :
  ctx(EVP_PKEY_CTX_new(pkey, e)), deallocate(true) {}


KeyContext::KeyContext(const KeyContext &kc) :
  ctx(EVP_PKEY_CTX_dup(kc.ctx)), deallocate(true) {}


KeyContext::KeyContext(EVP_PKEY_CTX *ctx, bool deallocate) :
  ctx(ctx), deallocate(deallocate) {}


KeyContext::~KeyContext() {
  if (ctx && deallocate) EVP_PKEY_CTX_free(ctx);
}


void KeyContext::setRSAPadding(padding_t _padding) {
  int padding;
  switch (_padding) {
  case NO_PADDING:         padding = RSA_NO_PADDING;         break;
  case PKCS1_PADDING:      padding = RSA_PKCS1_PADDING;      break;
#ifdef RSA_SSLV23_PADDING
  case SSLV23_PADDING:     padding = RSA_SSLV23_PADDING;     break;
#endif
  case PKCS1_OAEP_PADDING: padding = RSA_PKCS1_OAEP_PADDING; break;
  case X931_PADDING:       padding = RSA_X931_PADDING;       break;
  default: THROW("Invalid padding " << _padding);
  }

  if (EVP_PKEY_CTX_set_rsa_padding(ctx, padding) <= 0)
    THROW("Failed to set RSA padding: " << SSL::getErrorStr());
}


void KeyContext::setRSAPSSSaltLen(int len) {
  if (EVP_PKEY_CTX_set_rsa_pss_saltlen(ctx, len) <= 0)
    THROW("Failed to set RSA salt length: " << SSL::getErrorStr());
}


void KeyContext::setRSABits(int bits) {
  if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0)
    THROW("Failed to set RSA bits: " << SSL::getErrorStr());
}


void KeyContext::setRSAPubExp(uint64_t exp) {
  BigNum num;
  num.set(exp);

#if OPENSSL_VERSION_NUMBER < 0x3000000fL
  int ret = EVP_PKEY_CTX_set_rsa_keygen_pubexp(ctx, num.get());
#else
  int ret = EVP_PKEY_CTX_set1_rsa_keygen_pubexp(ctx, num.get());
#endif

  if (ret <= 0)
    THROW("Failed to set RSA public exponent: " << SSL::getErrorStr());

#if OPENSSL_VERSION_NUMBER < 0x3000000fL
  num.adopt(); // Don't deallocate
#endif
}


void KeyContext::setRSAOAEPMD(const string &digest) {
  if (EVP_PKEY_CTX_set_rsa_oaep_md(ctx, Digest::getAlgorithm(digest)) <= 0)
    THROW("Failed to set RSA OAEP MD: " << SSL::getErrorStr());

}


void KeyContext::setRSAMGF1MD(const string &digest) {
  if (EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, Digest::getAlgorithm(digest)) <= 0)
    THROW("Failed to set RSA MGF1 MD: " << SSL::getErrorStr());

}


void KeyContext::setDSABits(int bits) {
  if (EVP_PKEY_CTX_set_dsa_paramgen_bits(ctx, bits) <= 0)
    THROW("Failed to set DSA bits: " << SSL::getErrorStr());
}


void KeyContext::setDHPrimeLen(int len) {
  if (EVP_PKEY_CTX_set_dh_paramgen_prime_len(ctx, len) <= 0)
    THROW("Failed to set Diffie-Hellman prime length: " << SSL::getErrorStr());
}


void KeyContext::setDHGenerator(int gen) {
  if (EVP_PKEY_CTX_set_dh_paramgen_generator(ctx, gen) <= 0)
    THROW("Failed to set Diffie-Hellman generator: " << SSL::getErrorStr());
}


void KeyContext::setECCurve(const string &curve) {
  if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, SSL::findObject(curve)) <= 0)
    THROW("Failed to set Elliptic Curve curve: " << SSL::getErrorStr());
}


namespace {
  int keygen_cb(EVP_PKEY_CTX *ctx) {
    KeyGenCallback *cb = (KeyGenCallback *)EVP_PKEY_CTX_get_app_data(ctx);
    if (cb) (*cb)(EVP_PKEY_CTX_get_keygen_info(ctx, 0));
    return 1;
  }
}


void KeyContext::setKeyGenCallback(KeyGenCallback *callback) {
  EVP_PKEY_CTX_set_cb(ctx, callback ? keygen_cb : 0);
  EVP_PKEY_CTX_set_app_data(ctx, callback);
}


void KeyContext::setSignatureMD(const string &digest) {
  if (EVP_PKEY_CTX_set_signature_md(ctx, Digest::getAlgorithm(digest)) <= 0)
    THROW("Failed to set signature message digest to '" << digest << "': "
          << SSL::getErrorStr());
}


void KeyContext::keyGenInit() {
  if (EVP_PKEY_keygen_init(ctx) <= 0)
    THROW("Error initializing key context for key generation: "
           << SSL::getErrorStr());
}


KeyPair KeyContext::keyGen() const {
  EVP_PKEY *pkey = 0;

  if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    THROW("Error generating key: " << SSL::getErrorStr());

  return KeyPair(pkey);
}


void KeyContext::paramGenInit() {
  if (EVP_PKEY_paramgen_init(ctx) <= 0)
    THROW("Error initializing key context for parameter generation: "
           << SSL::getErrorStr());
}


KeyPair KeyContext::paramGen() const {
  EVP_PKEY *pkey = 0;

  if (EVP_PKEY_paramgen(ctx, &pkey) <= 0)
    THROW("Error generating parameters: " << SSL::getErrorStr());

  return KeyPair(pkey);
}


void KeyContext::signInit() {
  if (EVP_PKEY_sign_init(ctx) <= 0)
    THROW("Error initializing key context for signing: "
           << SSL::getErrorStr());
}


size_t KeyContext::sign(uint8_t *sigData, size_t sigLen,
                        const uint8_t *msgData, size_t msgLen) {
  if (EVP_PKEY_sign(ctx, sigData, &sigLen, msgData, msgLen) <= 0)
    THROW("Failed to "
           << (sigData ? "sign message" : "compute signature length") << ": "
           << SSL::getErrorStr());

  return sigLen;
}


string KeyContext::sign(const string &msg) {
  const uint8_t *msgData = (const uint8_t *)msg.c_str();
  size_t msgLen = msg.length();

  // Determine signature length
  size_t sigLen = sign(0, 0, msgData, msgLen);

  // Allocate space
  SmartPointer<uint8_t>::Array sigData = new uint8_t[sigLen];

  // Sign
  sign(sigData.get(), sigLen, msgData, msgLen);

  // Return result
  return string((char *)sigData.get(), sigLen);
}


void KeyContext::verifyInit() {
  if (EVP_PKEY_verify_init(ctx) <= 0)
    THROW("Error initializing key context for verification:"
           << SSL::getErrorStr());
}


void KeyContext::verify(const uint8_t *sigData, size_t sigLen,
                        const uint8_t *msgData, size_t msgLen) {
  switch (EVP_PKEY_verify(ctx, sigData, sigLen, msgData, msgLen)) {
  case 0: THROW("Failed to verify signature");
  case 1: return;
  default: THROW("Error verifying signature: " << SSL::getErrorStr());
  }
}


void KeyContext::verify(const string &sig, const string &msg) {
  verify((const uint8_t *)sig.data(), sig.length(),
         (const uint8_t *)msg.data(), msg.length());
}


void KeyContext::verifyRecoverInit() {
  if (EVP_PKEY_verify_recover_init(ctx) <= 0)
    THROW("Error initializing key context to recover signature data:"
           << SSL::getErrorStr());
}


size_t KeyContext::verifyRecover(uint8_t *msgData, size_t msgLen,
                                 const uint8_t *sigData, size_t sigLen) {
  if (EVP_PKEY_verify_recover(ctx, msgData, &msgLen, sigData, sigLen) <= 0)
    THROW("Failed to " << (msgData ? "recover signature data: " :
                            "compute data length: ") << SSL::getErrorStr());

  return msgLen;
}


string KeyContext::verifyRecover(const string &sig) {
  const uint8_t *sigData = (const uint8_t *)sig.c_str();
  size_t sigLen = sig.length();

  // Determine signature length
  size_t msgLen = verifyRecover(0, 0, sigData, sigLen);

  // Allocate space
  SmartPointer<uint8_t>::Array msgData = new uint8_t[msgLen];

  // Recover data
  verifyRecover(msgData.get(), msgLen, sigData, sigLen);

  // Return result
  return string((char *)msgData.get(), msgLen);
}


void KeyContext::encryptInit() {
  if (EVP_PKEY_encrypt_init(ctx) <= 0)
    THROW("Error initializing key context for encryption: "
          << SSL::getErrorStr());
}


size_t KeyContext::encrypt(uint8_t *outData, size_t outLen,
                           const uint8_t *inData, size_t inLen) {
  if (EVP_PKEY_encrypt(ctx, outData, &outLen, inData, inLen) <= 0)
    THROW("Failed to " << (outData ? "encrypt: " :
                            "compute data length: ") << SSL::getErrorStr());

  return outLen;
}


string KeyContext::encrypt(const string &in) {
  const uint8_t *inData = (const uint8_t *)in.c_str();
  size_t inLen = in.length();

  // Determine output length
  size_t outLen = encrypt(0, 0, inData, inLen);

  // Allocate space
  SmartPointer<uint8_t>::Array outData = new uint8_t[outLen];

  // Encrypt
  encrypt(outData.get(), outLen, inData, inLen);

  // Return result
  return string((char *)outData.get(), outLen);
}


void KeyContext::decryptInit() {
  if (EVP_PKEY_decrypt_init(ctx) <= 0)
    THROW("Error initializing key context for decryption: "
           << SSL::getErrorStr());
}


size_t KeyContext::decrypt(uint8_t *outData, size_t outLen,
                           const uint8_t *inData, size_t inLen) {
  if (EVP_PKEY_decrypt(ctx, outData, &outLen, inData, inLen) <= 0)
    THROW("Failed to " << (outData ? "decrypt: " :
                            "compute data length: ") << SSL::getErrorStr());

  return outLen;
}


string KeyContext::decrypt(const string &in) {
  const uint8_t *inData = (const uint8_t *)in.c_str();
  size_t inLen = in.length();

  // Determine output length
  size_t outLen = decrypt(0, 0, inData, inLen);

  // Allocate space
  SmartPointer<uint8_t>::Array outData = new uint8_t[outLen];

  // Decrypt
  decrypt(outData.get(), outLen, inData, inLen);

  // Return result
  return string((char *)outData.get(), outLen);
}


void KeyContext::deriveInit() {
  if (EVP_PKEY_derive_init(ctx) <= 0)
    THROW("Error initializing key context to derive shared secret: "
           << SSL::getErrorStr());
}


void KeyContext::setDerivePeer(const KeyPair &key) {
  if (EVP_PKEY_derive_set_peer(ctx, key.getEVP_PKEY()) <= 0)
    THROW("Failed to set derive peer: " << SSL::getErrorStr());
}


size_t KeyContext::derive(uint8_t *keyData, size_t keyLen) {
  if (EVP_PKEY_derive(ctx, keyData, &keyLen) <= 0)
    THROW("Failed to " << (keyData ? "derive shared secret: " :
                            "compute data length: ") << SSL::getErrorStr());

  return keyLen;
}


string KeyContext::derive() {
  // Determine key length
  size_t keyLen = derive(0, 0);

  // Allocate space
  SmartPointer<uint8_t>::Array keyData = new uint8_t[keyLen];

  // Derive shared secret
  derive(keyData.get(), keyLen);

  // Return result
  return string((char *)keyData.get(), keyLen);
}
