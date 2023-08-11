/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include <cbang/SmartPointer.h>
#include <cbang/net/Base64.h>

#include <istream>
#include <vector>
#include <cstdint>

typedef struct evp_md_st EVP_MD;
typedef struct evp_md_ctx_st EVP_MD_CTX;
typedef struct engine_st ENGINE;


namespace cb {
  class KeyPair;
  class KeyContext;

  class Digest {
    const EVP_MD *md;
    EVP_MD_CTX *ctx;
    bool initialized;
    std::vector<uint8_t> digest;

    // No copy
    Digest(const Digest &);
    Digest &operator=(const Digest &);

  public:
    Digest(const std::string &digest);
    virtual ~Digest();

    EVP_MD_CTX *getEVP_MD_CTX() const {return ctx;}

    virtual unsigned size() const;

    // Hash
    void init(ENGINE *e = 0);

    void update(std::istream &stream);
    void update(const std::string &data);
    virtual void update(const uint8_t *data, unsigned length);

    template <typename T>
    void updateWith(const T &o) {update((const uint8_t *)&o, sizeof(T));}

    virtual void finalize();

    // Sign
    SmartPointer<KeyContext> signInit(const KeyPair &key, ENGINE *e = 0);
    size_t sign(uint8_t *sigData, size_t sigLen);
    std::string sign();

    // Verify
    SmartPointer<KeyContext> verifyInit(const KeyPair &key, ENGINE *e = 0);
    bool verify(const uint8_t *sigData, size_t sigLen);
    bool verify(const std::string &sig);

    // Get hash
    std::string toString() const;
    std::string toString();
    std::string toHexString() const;
    std::string toHexString();
    std::string toBase64(const Base64 &base64 = Base64()) const;
    std::string toBase64(const Base64 &base64 = Base64());
    std::string toURLBase64() const;
    std::string toURLBase64();
    unsigned getDigest(uint8_t *buffer, unsigned length) const;
    unsigned getDigest(uint8_t *buffer, unsigned length);
    const std::vector<uint8_t> &getDigest() const {return digest;}
    const std::vector<uint8_t> &getDigest();

    // Reset
    virtual void reset();

    // Static
    static std::string hash(const std::string &s, const std::string &digest,
                            ENGINE *e = 0);
    static std::string hashHex(const std::string &s, const std::string &digest,
                               ENGINE *e = 0);
    static std::string base64(const std::string &s, const std::string &digest,
                              const Base64 &base64 = Base64(), ENGINE *e = 0);
    static std::string urlBase64(const std::string &s,
                                 const std::string &digest, ENGINE *e = 0);
    static std::string sign(const KeyPair &key, const std::string &s,
                            const std::string &digest, ENGINE *e = 0);
    static bool verify(const KeyPair &key, const std::string &s,
                       const std::string &sig, const std::string &digest,
                       ENGINE *e = 0);
    static bool hasAlgorithm(const std::string &digest);

    static std::string signHMAC(const std::string &key, const std::string &s,
                                const std::string &digest, ENGINE *e = 0);
    static bool verifyHMAC(const std::string &key, const std::string &s,
                           const std::string &sig, const std::string &digest,
                           ENGINE *e = 0);
  };
}
