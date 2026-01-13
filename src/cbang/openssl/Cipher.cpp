/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include "Cipher.h"
#include "SSL.h"

#include <cbang/Exception.h>

#include <openssl/evp.h>

#include <cstring>

using namespace std;
using namespace cb;


Cipher::Cipher(const string &algorithm, bool encrypt, const void *key,
               const void *iv, ENGINE *e) :
  ctx(EVP_CIPHER_CTX_new()) {
  reset();
  init(lookup(algorithm), encrypt, key, iv, e);
}


Cipher::Cipher(
  const string &algorithm, const string &key, const string &iv, ENGINE *e) :
  Cipher(algorithm, true, key.c_str(), iv.c_str()) {}


Cipher::~Cipher() {
  if (initialized) reset();
  if (ctx) EVP_CIPHER_CTX_free(ctx);
}


bool Cipher::isEncrypting() const {return EVP_CIPHER_CTX_encrypting(ctx);}
unsigned Cipher::getKeyLength() const {return EVP_CIPHER_CTX_key_length(ctx);}
void Cipher::setKey(const void *key) {init(0, isEncrypting(), key, 0, 0);}
unsigned Cipher::getIVLength() const {return EVP_CIPHER_CTX_iv_length(ctx);}
void Cipher::setIV(const void *iv) {init(0, isEncrypting(), 0, iv, 0);}
unsigned Cipher::getBlockSize() const {return EVP_CIPHER_CTX_block_size(ctx);}


void Cipher::reset() {
  if (ctx) EVP_CIPHER_CTX_reset(ctx);
  initialized = false;
}


void Cipher::init(const EVP_CIPHER *cipher, bool encrypt, const void *key,
                  const void *iv, ENGINE *e) {
  if (!EVP_CipherInit_ex(ctx, cipher, e, (const unsigned char *)key,
                         (const unsigned char *)iv, encrypt))
    THROW("Cipher init failed: " << SSL::getErrorStr());

  initialized = true;
}


unsigned Cipher::update(
  void *out, unsigned outLen, const void *in, unsigned inLen) {

  if (!initialized) THROW("Cipher not initialized");

  // Handle inplace operation
  SmartPointer<char>::Array buf;
  if (out == in) {
    buf = new char[inLen];
    memcpy(buf.get(), in, inLen);
    in = buf.get();
  }

  if (!EVP_CipherUpdate(
        ctx, (unsigned char *)out, (int *)&outLen, (unsigned char *)in, inLen))
    THROW("Error updating cipher: " << SSL::getErrorStr());

  return outLen;
}


unsigned Cipher::finalize(void *out, unsigned outLen) {
  if (!initialized) THROW("Cipher not initialized");

  if (!EVP_CipherFinal_ex(ctx, (unsigned char *)out, (int *)&outLen))
    THROW("Error finalizing cipher: " << SSL::getErrorStr());

  return outLen;
}


string Cipher::crypt(const string &data) {
  unsigned outLen = data.length() + getBlockSize();
  SmartPointer<char>::Array buf = new char[outLen];

  unsigned bytes = update(buf.get(), outLen, data.data(), data.length());
  bytes += finalize(buf.get() + bytes, outLen - bytes);

  return string(buf.get(), bytes);
}


const EVP_CIPHER *Cipher::lookup(const string &algorithm) {
  const EVP_CIPHER *c = EVP_get_cipherbyname(algorithm.c_str());
  if (!c) THROW("Unrecognized cipher '" << algorithm << "'");
  return c;
}
