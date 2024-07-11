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

#include "SSLContext.h"

#include "SSL.h"
#include "BIStream.h"
#include "KeyPair.h"
#include "Certificate.h"
#include "CertificateChain.h"
#include "CRL.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SysError.h>
#include <cbang/io/InputSource.h>

// This avoids a conflict with OCSP_RESPONSE in wincrypt.h
#ifdef OCSP_RESPONSE
#undef OCSP_RESPONSE
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/opensslv.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#endif

#ifdef __APPLE__
#include <Security/Security.h>
#include <cbang/os/osx/MacOSString.h>
#endif

using namespace std;
using namespace cb;


namespace {
  extern "C" {
    int verify_callback(int preverify_ok, X509_STORE_CTX *ctx) {
#ifdef DEBUG
      X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
      int err = X509_STORE_CTX_get_error(ctx);
      int depth = X509_STORE_CTX_get_error_depth(ctx);

      if (!preverify_ok) {
        char buf[256];
        X509_NAME_oneline(X509_get_subject_name(cert), buf, 256);

        LOG_DEBUG(4, "SSL verify error:" << err << ':' <<
                  X509_verify_cert_error_string(err) << ":depth="
                  << depth << buf);
      }
#endif

      return preverify_ok;
    }
  }
}


#ifdef __APPLE__
namespace cb { // avoid namespace collision with Apple Security.framework
#endif

SSLContext::SSLContext() : ctx(0) {reset();}


SSLContext::~SSLContext() {
  if (ctx) {
    SSL_CTX_free(ctx);
    ctx = 0;
  }
}


void SSLContext::reset() {
  if (ctx) SSL_CTX_free(ctx);

  ctx = SSL_CTX_new(TLS_method());
  if (!ctx) THROW("Failed to create SSL context: " << cb::SSL::getErrorStr());

  SSL_CTX_set_default_passwd_cb(ctx, cb::SSL::passwordCallback);

  // A session ID is required for session caching to work
  SSL_CTX_set_session_id_context(ctx, (unsigned char *)"cbang", 5);

  setVerifyNone();
}



X509_STORE *SSLContext::getStore() const {
  X509_STORE *store = SSL_CTX_get_cert_store(ctx);
  if (!store) THROW("Could not get SSL context certificate store");

  return store;
}


SmartPointer<cb::SSL> SSLContext::createSSL(BIO *bio) {
  return new cb::SSL(ctx, bio);
}


void SSLContext::setCipherList(const string &list) {
  if (!SSL_CTX_set_cipher_list(ctx, list.c_str()))
    THROW("Failed to set cipher list to: " << list
          << ": " << cb::SSL::getErrorStr());
}


void SSLContext::setVerifyNone() {
  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, verify_callback);
}


void SSLContext::setVerifyPeer(bool verifyClientOnce, bool failIfNoPeerCert,
                               unsigned depth) {
  int mode = SSL_VERIFY_PEER;
  if (verifyClientOnce) mode |= SSL_VERIFY_CLIENT_ONCE;
  if (failIfNoPeerCert) mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  SSL_CTX_set_verify(ctx, mode, verify_callback);
  if (depth) SSL_CTX_set_verify_depth(ctx, depth);
}


void SSLContext::addClientCA(const Certificate &cert) {
  if (!SSL_CTX_add_client_CA(ctx, cert.getX509()))
    THROW("Failed to add client CA: " << cb::SSL::getErrorStr());
}


void SSLContext::useCertificate(const Certificate &cert) {
  if (!SSL_CTX_use_certificate(ctx, cert.getX509()))
    THROW("Failed to use certificate: " << cb::SSL::getErrorStr());
}


void SSLContext::addExtraChainCertificate(const Certificate &cert) {
  if (!SSL_CTX_add_extra_chain_cert(ctx, X509_dup(cert.getX509())))
    THROW("Failed to add extra chain certificate: " << cb::SSL::getErrorStr());
}


void SSLContext::clearExtraChainCertificates() {
  if (!SSL_CTX_clear_extra_chain_certs(ctx))
    THROW("Failed to clear extra chain certificates: "
          << cb::SSL::getErrorStr());
}


void SSLContext::useCertificateChainFile(const string &filename) {
  if (!(SSL_CTX_use_certificate_chain_file(ctx, filename.c_str())))
    THROW("Failed to load certificate chain file '" << filename
           << "': " << cb::SSL::getErrorStr());
}


void SSLContext::useCertificateChain(const CertificateChain &chain) {
  if (!chain.size()) THROW("Empty certificate chain");

  clearExtraChainCertificates();

  for (unsigned i = 1; i < chain.size(); i++)
    addExtraChainCertificate(chain.get(i));

  useCertificate(chain.get(0));
}


void SSLContext::usePrivateKey(const KeyPair &key) {
  if (!(SSL_CTX_use_PrivateKey(ctx, key.getEVP_PKEY())))
    THROW("Failed to use private key: " << cb::SSL::getErrorStr());
}


void SSLContext::usePrivateKey(istream &stream) {
  KeyPair pri;
  pri.readPrivatePEM(stream);
  usePrivateKey(pri);
}


void SSLContext::addTrustedCA(const Certificate &cert) {
  X509_STORE *store = getStore();
  if (!X509_STORE_add_cert(store, cert.getX509()))
    THROW("Failed to add certificate to store " << cb::SSL::getErrorStr());
}


void SSLContext::addTrustedCAStr(const string &data) {
  addTrustedCA(InputSource(data));
}


void SSLContext::addTrustedCA(istream &stream) {
  BIStream bio(stream);
  addTrustedCA(bio.getBIO());
}


void SSLContext::addTrustedCA(BIO *bio) {
  // TODO free X509
  X509 *cert = PEM_read_bio_X509(bio, 0, cb::SSL::passwordCallback, 0);
  if (!cert) THROW("Failed to read certificate " << cb::SSL::getErrorStr());

  X509_STORE *store = getStore();
  if (!X509_STORE_add_cert(store, cert))
    THROW("Failed to add certificate to store " << cb::SSL::getErrorStr());
}


void SSLContext::loadVerifyLocationsFile(const string &path) {
  if (!SSL_CTX_load_verify_locations(ctx, path.c_str(), 0))
    THROW("Failed to load verify locations file '" << path << "': "
          << cb::SSL::getErrorStr());
}


void SSLContext::loadVerifyLocationsPath(const string &path) {
  if (!SSL_CTX_load_verify_locations(ctx, 0, path.c_str()))
    THROW("Failed to load verify locations path '" << path << "'");
}


void SSLContext::loadSystemRootCerts() {
#ifdef _WIN32
  // Use Wincrypt API to get Windows root certificates
  X509_STORE *verifyStore = SSL_CTX_get_cert_store(ctx);
  HCERTSTORE store = CertOpenSystemStoreA(0, "ROOT");
  if (!store) THROW("Error opening system root cert store: " << SysError());

  PCCERT_CONTEXT cctx = 0;
  while ((cctx = CertEnumCertificatesInStore(store, cctx))) {
    const uint8_t *bytes = (const uint8_t *)cctx->pbCertEncoded;
    X509 *cert = d2i_X509(0, &bytes, cctx->cbCertEncoded);

    if (!cert) {
      LOG_WARNING("Error parsing system root cert: " << SSL::getErrorStr());
      continue;
    }

    if (!X509_STORE_add_cert(verifyStore, cert)) {
      auto error = ERR_get_error();

      if (ERR_GET_REASON(error) != X509_R_CERT_ALREADY_IN_HASH_TABLE) {
        LOG_ERROR("Error adding system root cert: " << SSL::getErrorStr(error));
        X509_free(cert);
        break;
      }
    }

    X509_free(cert);
  }

  CertCloseStore(store, 0);

#elif __APPLE__
  X509_STORE *verifyStore = SSL_CTX_get_cert_store(ctx);
  CFArrayRef anchors      = 0;
  CFIndex trustedCount    = 0;

  OSStatus err = SecTrustCopyAnchorCertificates(&anchors);
  if (err != errSecSuccess) {
    LOG_ERROR(CBANG_FUNC << "() Failed to get anchor certificates");
    return;
  }

  CFIndex count = CFArrayGetCount(anchors);
  LOG_DEBUG(3, "Anchor certificate count: " << count);

  for (CFIndex i = 0; i < count; i++) {
    SecCertificateRef cert;
    string summary = string();
    bool valid;

    cert = (SecCertificateRef)CFArrayGetValueAtIndex(anchors, i);

    MacOSString summary0(SecCertificateCopySubjectSummary(cert));
    if (summary0) summary = summary0;

    LOG_DEBUG(5, "Cert " << i << ": " << summary);

    // validate root cert
    // anchor certs are already trusted, but can be expired, revoked, or
    // explicitly distrusted by a user
    // on macOS 10.11 and earlier, some root certs will have expired
    valid = false;

    SecTrustRef trust       = 0;
    SecPolicyRef x509Policy = SecPolicyCreateBasicX509();
    if (x509Policy) {
      err = SecTrustCreateWithCertificates(cert, x509Policy, &trust);
      if (err == errSecSuccess)
        err = SecTrustSetPolicies(trust, x509Policy);

      // OPTIONAL, apparently, for our use
      // set params for validating a root cert
      // see ValidateSystemTrust in
      // https://github.com/macports/macports-ports/blob/master/security
      //   /certsync/files/certsync.m

      if (err == errSecSuccess) {
#if __clang__
        if (__builtin_available(macOS 10.14, *)) { // macOS 10.14+
#else
        if (Version(10, 14) <= MacOSUtilities::getOSVersion()) {
#endif
          valid = SecTrustEvaluateWithError(trust, 0);
          LOG_DEBUG(5, "Called SecTrustEvaluateWithError()");

        } else {
#if (MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_15)
          SecTrustResultType result;
          err = SecTrustEvaluate(trust, &result); // macOS 10.3–10.15
          LOG_DEBUG(5, "Called SecTrustEvaluate()");

          if (err == errSecSuccess) {
            switch (result) {
              case kSecTrustResultUnspecified:
              case kSecTrustResultProceed:
                valid = true;
                break;

              default: valid = false; break;
            }
          }
#endif
        }
      }

      if (x509Policy) CFRelease(x509Policy);
      if (trust) CFRelease(trust);
    }

    if (valid) {
      LOG_DEBUG(5, "Trusting certificate: " << summary);
      // convert and add cert to trusted store
      CFDataRef data = SecCertificateCopyData(cert);
      if (data) {
        const uint8_t *bytes = CFDataGetBytePtr(data);
        long len = CFDataGetLength(data);
        X509 *xcert = d2i_X509(0, &bytes, len);

        if (!xcert) {
          LOG_WARNING("Error parsing root cert: " << SSL::getErrorStr());
          CFRelease(data);
          continue;
        }

        if (!X509_STORE_add_cert(verifyStore, xcert)) {
          auto error = ERR_get_error();

          if (ERR_GET_REASON(error) != X509_R_CERT_ALREADY_IN_HASH_TABLE) {
            LOG_ERROR("Error adding root cert: " << SSL::getErrorStr(error));
            X509_free(xcert);
            CFRelease(data);
            break;
          }
        }
        trustedCount++;
        X509_free(xcert);
        CFRelease(data);
      } else LOG_ERROR("Failed to get data for certificate: " << summary);
    } else LOG_DEBUG(3, "Untrusted certificate: " << summary);
  }
  if (anchors) CFRelease(anchors);
  LOG_DEBUG(3, "Trusted certificate count: " << trustedCount);

#else
  if (!SSL_CTX_set_default_verify_paths(ctx))
    LOG_WARNING("System root certificates not found");
#endif
}


void SSLContext::addCRL(const CRL &crl) {
  X509_STORE *store = getStore();

  // Add CRL
  if (!X509_STORE_add_crl(store, crl.getX509_CRL()))
    THROW("Error adding CRL" << cb::SSL::getErrorStr());

  setCheckCRL(true);
}


void SSLContext::addCRLStr(const string &data) {
  addCRL(InputSource(data));
}


void SSLContext::addCRL(istream &stream) {
  BIStream bio(stream);
  addCRL(bio.getBIO());
}


void SSLContext::addCRL(BIO *bio) {
  X509_STORE *store = getStore();

  // Read CRL
  X509_CRL *crl = PEM_read_bio_X509_CRL(bio, 0, 0, 0);
  if (!crl || cb::SSL::peekError())
    THROW("Error reading CRL " << cb::SSL::getErrorStr());

  // Add CRL
  if (!X509_STORE_add_crl(store, crl))
    THROW("Error adding CRL" << cb::SSL::getErrorStr());

  setCheckCRL(true);
}


void SSLContext::setVerifyDepth(unsigned depth) {
  SSL_CTX_set_verify_depth(ctx, depth);
}


void SSLContext::setCheckCRL(bool x) {
  X509_STORE *store = getStore();

  // Enable CRL checking
  X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
  X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
  X509_STORE_set1_param(store, param);
  X509_VERIFY_PARAM_free(param);
}


long SSLContext::getOptions() const {return SSL_CTX_get_options(ctx);}
void SSLContext::setOptions(long options) {SSL_CTX_set_options(ctx, options);}

#ifdef __APPLE__
} // namespace cb
#endif
