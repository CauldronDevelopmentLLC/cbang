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

#pragma once

#include "BigNum.h"
#include "KeyGenCallback.h"
#include "PasswordCallback.h"

#include <cbang/SmartPointer.h>

#include <iostream>
#include <string>
#include <cstdint>

typedef struct evp_pkey_st EVP_PKEY;
typedef struct engine_st ENGINE;


namespace cb {
  class KeyPair {
    EVP_PKEY *key;

  public:
    typedef enum {
      HMAC_KEY,
      CMAC_KEY,
    } mac_key_t;

    KeyPair(const KeyPair &o);
    KeyPair(EVP_PKEY *key) : key(key) {}
    KeyPair(const std::string &key, mac_key_t type = HMAC_KEY, ENGINE *e = 0);
    KeyPair();
    ~KeyPair();

    const KeyPair &operator=(const KeyPair &o);

    EVP_PKEY *getEVP_PKEY() const {return key;}
    void setEVP_PKEY(EVP_PKEY *key) {this->key = key;}

    bool isValid() const;
    bool isRSA() const;
    bool isDSA() const;
    bool isDH() const;
    bool isEC() const;

    BigNum getParam(const char *id) const;
    BigNum getRSA_E() const;
    BigNum getRSA_N() const;

    BigNum getPrivate() const;
    BigNum getPublic() const;

    bool hasPrivate() const;
    bool hasPublic() const;

    unsigned size() const; ///< In bytes
    bool match(const KeyPair &o) const;

    // Generate
    void generateRSA(unsigned bits = 4096, uint64_t pubExp = 65537,
                     SmartPointer<KeyGenCallback> callback = 0);
    void generateDSA(unsigned bits = 4096,
                     SmartPointer<KeyGenCallback> callback = 0);
    void generateDH(unsigned primeLen, int generator = 2,
                    SmartPointer<KeyGenCallback> callback = 0);
    void generateEC(const std::string &curve = "secp192k1",
                    SmartPointer<KeyGenCallback> callback = 0);

    // To DER string
    std::string publicToDER() const;
    std::string privateToDER() const;

    // To PEM string
    std::string publicToString() const;
    std::string privateToString() const;
    std::string toString() const;

    // Read PEM
    std::istream &readPublic(std::istream &stream);
    void readPublic(const std::string &pem);
    std::istream &readPrivate(std::istream &stream,
                              SmartPointer<PasswordCallback> callback = 0);
    void readPrivate(const std::string &pem,
                     SmartPointer<PasswordCallback> callback = 0);
    void read(const std::string &pem,
              SmartPointer<PasswordCallback> callback = 0);
    std::istream &read(std::istream &stream,
                       SmartPointer<PasswordCallback> callback = 0);

    // Write PEM
    std::ostream &writePublic(std::ostream &stream) const;
    std::ostream &writePrivate(std::ostream &stream) const;
    std::ostream &write(std::ostream &stream) const;

    // Print ASN1
    std::ostream &printPublic(std::ostream &stream, int indent = 0) const;
    std::ostream &printPrivate(std::ostream &stream, int indent = 0) const;
    std::ostream &print(std::ostream &stream, int indent = 0) const;

    // Signatures
    std::string sign(const std::string &data) const;
    std::string signSHA256(const std::string &data) const;
    std::string signBase64SHA256(const std::string &data) const;
    void verify(const std::string &signature, const std::string &data) const;
    void verifyBase64SHA256(const std::string &sig64,
                            const std::string &data) const;
  };


  inline std::istream &operator>>(std::istream &stream, KeyPair &key) {
    return key.read(stream);
  }

  inline std::ostream &operator<<(std::ostream &stream, const KeyPair &key) {
    return key.write(stream);
  }
}
