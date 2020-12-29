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

#include <cbang/config.h>
#include <cbang/SmartPointer.h>

#include <string>
#include <vector>

#ifdef HAVE_OPENSSL
typedef struct ssl_st _SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct bio_st BIO;


namespace cb {
  class Mutex;
  class BStream;
  class Certificate;

  class SSL {
    _SSL *ssl;
    unsigned handshakes = 0;

    static bool initialized;
    static Mutex *locks;
    static unsigned maxHandshakes;

    enum {
      PROCEED,
      WANTS_ACCEPT,
      WANTS_CONNECT,
    } state = PROCEED;

    int lastErr = 0;

  public:
    SSL(_SSL *ssl);
    SSL(const SSL &ssl);
    SSL(SSL_CTX *ctx, BIO *bio = 0);
    ~SSL();

    _SSL *getSSL() const {return ssl;}
    void setBIO(BIO *bio);

    void setFD(int fd);
    void setReadFD(int fd);
    void setWriteFD(int fd);

    bool wantsRead() const;
    bool wantsWrite() const;

    void setCipherList(const std::string &list);

    std::string getFullSSLErrorStr(int ret = 0) const;
    bool hasPeerCertificate() const;
    void verifyPeerCertificate() const;
    SmartPointer<Certificate> getPeerCertificate() const;
    void setTLSExtHostname(const std::string &hostname);

    void setConnectState();
    void setAcceptState();

    void connect();
    void accept();
    void shutdown();

    unsigned getPending() const;
    int read(char *data, unsigned size);
    unsigned write(const char *data, unsigned size);

    static unsigned getMaxHandshakes() {return maxHandshakes;}
    static void setMaxHandshakes(unsigned n) {maxHandshakes = n;}

    static unsigned long idCallback();
    static void lockingCallback(int mode, int n, const char *file, int line);
    static int passwordCallback(char *buf, int num, int rwflags, void *data);

    static void flushErrors();
    static unsigned getError();
    static unsigned peekError();
    static unsigned peekLastError();
    static std::string getErrorStr(unsigned err = 0);
    static const char *getSSLErrorStr(int err);

    // TODO move the following functions to a more generic class
    static int createObject(const std::string &oid,
                            const std::string &shortName,
                            const std::string &longName);
    static int findObject(const std::string &name);

    static void init();
    static void deinit();

    void infoCallback(int where, int ret);
    void limitRenegotiation();

  protected:
    void checkHandshakes();
    bool checkWants();
    void checkError(int ret);
  };


  CBANG_DEFINE_EXCEPTION_SUBCLASS(SSLException);
}

#else // HAVE_OPENSSL
namespace cb {class SSL {};}
#endif // HAVE_OPENSSL
