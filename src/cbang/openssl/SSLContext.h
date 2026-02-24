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

#pragma once

#include <cbang/config.h>
#include <cbang/SmartPointer.h>

#include <istream>
#include <string>

#ifdef HAVE_OPENSSL
typedef struct ssl_ctx_st SSL_CTX;
typedef struct x509_store_st X509_STORE;
typedef struct bio_st BIO;

namespace cb {
  class SSL;
  class KeyPair;
  class Certificate;
  class CertificateChain;
  class CRL;

  class SSLContext {
    SSL_CTX *ctx;

  public:
    SSLContext();
    ~SSLContext();

    void reset();

    SSL_CTX *getCTX() const {return ctx;}
    X509_STORE *getStore() const;

    SmartPointer<SSL> createSSL(BIO *bio = 0);

    void setCipherList(const std::string &list);

    void setVerifyNone();
    void setVerifyPeer(bool verifyClientOnce = true,
                       bool failIfNoPeerCert = false, unsigned depth = 1);

    void addClientCA(const Certificate &cert);

    void useCertificate(const Certificate &cert);
    void addExtraChainCertificate(const Certificate &cert);
    void clearExtraChainCertificates();
    void useCertificateChainFile(const std::string &filename);
    void useCertificateChain(const CertificateChain &chain);
    void usePrivateKey(const KeyPair &key);
    void usePrivateKey(std::istream &stream);

    void addTrustedCA(const Certificate &cert);
    void addTrustedCAStr(const std::string &data);
    void addTrustedCA(std::istream &stream);
    void addTrustedCA(BIO *bio);

    void loadVerifyLocationsFile(const std::string &path);
    void loadVerifyLocationsPath(const std::string &path);
    void loadSystemRootCerts();

    void addCRL(const CRL &crl);
    void addCRLStr(const std::string &data);
    void addCRL(std::istream &stream);
    void addCRL(BIO *bio);

    void setVerifyDepth(unsigned depth);
    void setCheckCRL(bool x = true);
    void setSessionTimeout(unsigned secs);
    void setSessionCacheSize(unsigned size);
    long getSessionCacheMode();
    void setSessionCacheMode(long mode);

    long getOptions() const;
    void setOptions(long options);

    enum option_t {
      SSL_OP_NO_EXTENDED_MASTER_SECRET                = 1UL << 0,
      SSL_OP_CLEANSE_PLAINTEXT                        = 1UL << 1,
      SSL_OP_LEGACY_SERVER_CONNECT                    = 1UL << 2,
      SSL_OP_ENABLE_KTLS                              = 1UL << 3,
      SSL_OP_TLSEXT_PADDING                           = 1UL << 4,
      SSL_OP_SAFARI_ECDHE_ECDSA_BUG                   = 1UL << 6,
      SSL_OP_IGNORE_UNEXPECTED_EOF                    = 1UL << 7,
      SSL_OP_ALLOW_CLIENT_RENEGOTIATION               = 1UL << 8,
      SSL_OP_DISABLE_TLSEXT_CA_NAMES                  = 1UL << 9,
      SSL_OP_ALLOW_NO_DHE_KEX                         = 1UL << 10,
      SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS              = 1UL << 11,
      SSL_OP_NO_QUERY_MTU                             = 1UL << 12,
      SSL_OP_COOKIE_EXCHANGE                          = 1UL << 13,
      SSL_OP_NO_TICKET                                = 1UL << 14,
      SSL_OP_CISCO_ANYCONNECT                         = 1UL << 15,
      SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION   = 1UL << 16,
      SSL_OP_NO_COMPRESSION                           = 1UL << 17,
      SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION        = 1UL << 18,
      SSL_OP_NO_ENCRYPT_THEN_MAC                      = 1UL << 19,
      SSL_OP_ENABLE_MIDDLEBOX_COMPAT                  = 1UL << 20,
      SSL_OP_PRIORITIZE_CHACHA                        = 1UL << 21,
      SSL_OP_CIPHER_SERVER_PREFERENCE                 = 1UL << 22,
      SSL_OP_TLS_ROLLBACK_BUG                         = 1UL << 23,
      SSL_OP_NO_ANTI_REPLAY                           = 1UL << 24,
      SSL_OP_NO_SSLv3                                 = 1UL << 25,
      SSL_OP_NO_TLSv1                                 = 1UL << 26,
      SSL_OP_NO_TLSv1_2                               = 1UL << 27,
      SSL_OP_NO_TLSv1_1                               = 1UL << 28,
      SSL_OP_NO_TLSv1_3                               = 1UL << 29,
      SSL_OP_NO_DTLSv1                                = 1UL << 26,
      SSL_OP_NO_DTLSv1_2                              = 1UL << 27,
      SSL_OP_NO_RENEGOTIATION                         = 1UL << 30,
      SSL_OP_CRYPTOPRO_TLSEXT_BUG                     = 1UL << 31,
      SSL_OP_NO_TX_CERTIFICATE_COMPRESSION            = 1UL << 32,
      SSL_OP_NO_RX_CERTIFICATE_COMPRESSION            = 1UL << 33,
      SSL_OP_ENABLE_KTLS_TX_ZEROCOPY_SENDFILE         = 1UL << 34,
      SSL_OP_PREFER_NO_DHE_KEX                        = 1UL << 35,
    };

    enum cache_mode_t {
      SSL_SESS_CACHE_OFF                              = 0x000,
      SSL_SESS_CACHE_CLIENT                           = 0x001,
      SSL_SESS_CACHE_SERVER                           = 0x002,
      SSL_SESS_CACHE_BOTH                             = 0x003,
      SSL_SESS_CACHE_NO_AUTO_CLEAR                    = 0x080,
      SSL_SESS_CACHE_NO_INTERNAL_LOOKUP               = 0x100,
      SSL_SESS_CACHE_NO_INTERNAL_STORE                = 0x200,
      SSL_SESS_CACHE_NO_INTERNAL                      = 0x300,
      SSL_SESS_CACHE_UPDATE_TIME                      = 0x400,
    };
  };
}

#else // HAVE_OPENSSL
namespace cb {class SSLContext {};}
#endif // HAVE_OPENSSL
