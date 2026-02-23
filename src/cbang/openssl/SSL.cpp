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

#include "SSL.h"

#include "SecurityUtilities.h"
#include "Certificate.h"
#include "ErrorSentry.h"

#include <cbang/config.h>
#include <cbang/log/Logger.h>

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#include <iostream>
#include <cstring>

// This avoids a conflict with OCSP_RESPONSE in wincrypt.h
#ifdef OCSP_RESPONSE
#undef OCSP_RESPONSE
#endif

#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>

#if !defined(OPENSSL_THREADS)
#error "OpenSSL must be compiled with thread support"
#endif

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>

#if 0x3000000fL <= OPENSSL_VERSION_NUMBER
#include <openssl/provider.h>
#endif


using namespace std;
using namespace cb;


#undef CBANG_EXCEPTION
#define CBANG_EXCEPTION SSLException


bool cb::SSL::initialized = false;


cb::SSL::SSL(_SSL *ssl) : ssl(ssl) {
  if (ssl) SSL_up_ref(ssl);
}


cb::SSL::SSL(const SSL &ssl) : SSL(ssl.ssl) {}


cb::SSL::SSL(SSL_CTX *ctx, BIO *bio) {
  ssl = SSL_new(ctx);
  if (!ssl) THROW("Failed to create new SSL: " << getErrorStr());
  if (bio) setBIO(bio);
}


cb::SSL::~SSL() {if (ssl) SSL_free(ssl);}


void cb::SSL::setBIO(BIO *bio) {SSL_set_bio(ssl, bio, bio);}


void cb::SSL::setFD(int fd) {
  if (!SSL_set_fd(ssl, fd))
  THROW("Failed to set SSL FD to " << fd << ": " << getErrorStr());
}


void cb::SSL::setReadFD(int fd) {
  if (!SSL_set_rfd(ssl, fd))
    THROW("Failed to set SSL read FD to " << fd << ": " << getErrorStr());
}


void cb::SSL::setWriteFD(int fd) {
  if (!SSL_set_wfd(ssl, fd))
    THROW("Failed to set SSL write FD to " << fd << ": " << getErrorStr());
}


bool cb::SSL::wantsRead()  const {return lastErr == SSL_ERROR_WANT_READ;}
bool cb::SSL::wantsWrite() const {return lastErr == SSL_ERROR_WANT_WRITE;}


void cb::SSL::setCipherList(const string &list) {
  if (!SSL_set_cipher_list(ssl, list.c_str()))
    THROW("Failed to set cipher list to: " << list << ": " << getErrorStr());
}


string cb::SSL::getFullSSLErrorStr(int ret) const {
  string s = getSSLErrorStr(SSL_get_error(ssl, ret));
  if (peekError()) s += ", " + getErrorStr();
  return s;
}


bool cb::SSL::hasPeerCertificate() const {
  X509 *cert = SSL_get_peer_certificate(ssl);
  if (!cert) return false;
  X509_free(cert);
  return true;
}


void cb::SSL::verifyPeerCertificate() const {
  if (!hasPeerCertificate()) THROW("Peer did not present a certificate");

  if (SSL_get_verify_result(ssl) != X509_V_OK)
    THROW("Certificate does not verify: " << getErrorStr());
}


SmartPointer<Certificate> cb::SSL::getPeerCertificate() const {
  if (SSL_get_verify_result(ssl) != X509_V_OK)
    THROW("Certificate does not verify: " << getErrorStr());

  X509 *cert = SSL_get_peer_certificate(ssl);
  if (!cert) THROW("Peer did not present a certificate");

  return new Certificate(cert);
}


vector<SmartPointer<Certificate> > cb::SSL::getVerifiedChain() const {
  vector<SmartPointer<Certificate> > certs;

  auto stack = SSL_get0_verified_chain(ssl);
  if (stack)
    for (int i = 0; i < sk_X509_num(stack); i++) {
      X509 *cert = sk_X509_value(stack, i);
      X509_up_ref(cert);
      certs.push_back(new Certificate(cert));
    }

  return certs;
}


void cb::SSL::setTLSExtHostname(const string &hostname) {
  if (!SSL_set_tlsext_host_name(ssl, hostname.c_str()))
    THROW("Failed to set TLS host name extension to '" << hostname
      << "': " << getErrorStr());
}


void cb::SSL::setConnectState() {SSL_set_connect_state(ssl);}
void cb::SSL::setAcceptState()  {SSL_set_accept_state(ssl);}


void cb::SSL::connect() {
  LOG_DEBUG(5, CBANG_FUNC << "()");
  ErrorSentry sentry;

  lastErr = 0;
  int ret = SSL_connect(ssl);

  if (ret == -1) {
    lastErr = SSL_get_error(ssl, ret);
    if (lastErr == SSL_ERROR_WANT_READ || lastErr == SSL_ERROR_WANT_WRITE) {
      state = WANTS_CONNECT;
      return;
    }
  }

  state = PROCEED;

  if (ret != 1) THROW("SSL connect failed: " << getFullSSLErrorStr(ret));
}


void cb::SSL::accept() {
  LOG_DEBUG(5, CBANG_FUNC << "()");
  ErrorSentry sentry;

  lastErr = 0;
  int ret = SSL_accept(ssl);

  if (ret == -1) {
    lastErr = SSL_get_error(ssl, ret);
    if (lastErr == SSL_ERROR_WANT_READ || lastErr == SSL_ERROR_WANT_WRITE) {
      state = WANTS_ACCEPT;
      LOG_DEBUG(5, CBANG_FUNC << "() wants "
                << (lastErr == SSL_ERROR_WANT_READ ? "read" : "write"));
      return;
    }
  }

  state = PROCEED;

  // We can ignore WANT_READ etc. here because the Socket layer already
  // retries reads and writes up to the specified timeout.
  if (ret != 1) {
    string err = getFullSSLErrorStr(ret);
    LOG_DEBUG(5, "SSL accept failed: " << err);
    THROW("SSL accept failed: " << err);
  }
}


void cb::SSL::shutdown() {
  ErrorSentry sentry;
  SSL_shutdown(ssl);
  LOG_DEBUG(5, CBANG_FUNC << "() " << getErrorStr());
}


unsigned cb::SSL::getPending() const {return SSL_pending(ssl);}


int cb::SSL::read(char *data, unsigned size) {
  LOG_DEBUG(5, CBANG_FUNC << "(size=" << size << ')');
  ErrorSentry sentry;

  lastErr = 0;
  if (!checkWants() || !size) return 0;

  int ret = SSL_read(ssl, data, size);
  if (ret <= 0) {
    // Detect End of Stream
    if (SSL_get_shutdown(ssl) == SSL_RECEIVED_SHUTDOWN) return -1;

    lastErr = SSL_get_error(ssl, ret);
    if (lastErr == SSL_ERROR_WANT_READ || lastErr == SSL_ERROR_WANT_WRITE) {
      LOG_DEBUG(5, CBANG_FUNC << "() wants "
                << (lastErr == SSL_ERROR_WANT_READ ? "read" : "write"));
      return 0;
    }

    string errMsg = getFullSSLErrorStr(lastErr);
    LOG_DEBUG(5, CBANG_FUNC << "() " << errMsg);
    THROW("SSL read failed: " << errMsg);
  }

  LOG_DEBUG(5, CBANG_FUNC << "()=" << ret);

#ifdef VALGRIND_MAKE_MEM_DEFINED
  (void)VALGRIND_MAKE_MEM_DEFINED(data, ret);
#endif // VALGRIND_MAKE_MEM_DEFINED

  return (unsigned)ret;
}


unsigned cb::SSL::write(const char *data, unsigned size) {
  LOG_DEBUG(5, CBANG_FUNC << "(size=" << size << ')');
  ErrorSentry sentry;

  lastErr = 0;
  if (!checkWants() || !size) return 0;

  int ret = SSL_write(ssl, data, size);
  if (ret <= 0) {
    lastErr = SSL_get_error(ssl, ret);
    if (lastErr == SSL_ERROR_WANT_READ || lastErr == SSL_ERROR_WANT_WRITE)
      return 0;
    THROW("SSL write failed: " << getFullSSLErrorStr(lastErr));
  }

  LOG_DEBUG(5, CBANG_FUNC << "()=" << ret);
  return (unsigned)ret;
}


void cb::SSL::init() {
  if (initialized) return;

#if 0x3000000fL <= OPENSSL_VERSION_NUMBER
  loadProvider("default");
  loadProvider("base");
#endif

  initialized = true;
}


int cb::SSL::passwordCallback(char *buf, int num, int rwflags, void *data) {
  string msg = "Enter ";
  if (rwflags) msg += "encryption";
  else msg += "decryption";
  msg += " password: ";

  string pass = SecurityUtilities::getpass(msg);

  strncpy(buf, pass.data(), pass.length());
  return pass.length();
}


void cb::SSL::flushErrors()       {ERR_clear_error();}
unsigned cb::SSL::getError()      {return ERR_get_error();}
unsigned cb::SSL::peekError()     {return ERR_peek_error();}
unsigned cb::SSL::peekLastError() {return ERR_peek_last_error();}


string cb::SSL::getErrorStr(unsigned err) {
  string result;
  char buffer[256];

  while (true) {
    if (!err) err = getError();
    if (!err) break;

    ERR_error_string(err, buffer);
    err = 0;

    if (!result.empty()) result += ", ";
    result += buffer;
  }

  return result;
}


const char *cb::SSL::getSSLErrorStr(int err) {
  switch (err) {
  case SSL_ERROR_NONE:             return "OK";
  case SSL_ERROR_SSL:              return "SSL";
  case SSL_ERROR_WANT_READ:        return "Want read";
  case SSL_ERROR_WANT_WRITE:       return "Want write";
  case SSL_ERROR_WANT_X509_LOOKUP: return "Want X509 lookup";
  case SSL_ERROR_SYSCALL:          return "Syscall";
  case SSL_ERROR_ZERO_RETURN:      return "Zero return";
  case SSL_ERROR_WANT_CONNECT:     return "Want connect";
  case SSL_ERROR_WANT_ACCEPT:      return "Want accept";
  default:                         return "Unknown";
  }
}


int cb::SSL::createObject(const string &oid, const string &shortName,
                          const string &longName) {
  int nid = OBJ_create(oid.c_str(), shortName.c_str(), longName.c_str());

  if (nid == NID_undef)
    THROW("Failed to create SSL object oid='" << oid << ", SN='" << shortName
           << "', LN='" << longName << "': " << getErrorStr());

  return nid;
}


int cb::SSL::findObject(const string &name) {
  int nid = OBJ_txt2nid(name.c_str());
  if (nid == NID_undef) THROW("Unrecognized SSL object '" << name << "'");
  return nid;
}


void cb::SSL::loadProvider(const string &provider) {
#if 0x3000000fL <= OPENSSL_VERSION_NUMBER
  if (!OSSL_PROVIDER_load(0, provider.c_str()))
    THROW("Failed to load SSL provider: " << provider << ": " << getErrorStr());
#endif
}


bool cb::SSL::checkWants() {
  switch (state) {
  case WANTS_ACCEPT:  accept();  break;
  case WANTS_CONNECT: connect(); break;
  default: break;
  }

  return state == PROCEED;
}
