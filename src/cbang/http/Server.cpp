/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Server.h"
#include "RequestErrorHandler.h"
#include "ConnIn.h"
#include "Request.h"

#include <cbang/config.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/config/Options.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/openssl/SSLContext.h>

#include <cinttypes>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


Server::Server(Event::Base &base, const SmartPointer<SSLContext> &sslCtx) :
  Event::Server(base), sslCtx(sslCtx) {}


void Server::addListenPort(const SockAddr &addr) {
  LOG_INFO(2, "Listening for HTTP on " << addr);
  bind(addr, 0, priority);
}


void Server::addSecureListenPort(const SockAddr &addr) {
  LOG_INFO(2, "Listening for HTTPS on " << addr);
  bind(addr, sslCtx, securePriority < 0 ? priority : securePriority);
}


void Server::addOptions(Options &options) {
  Event::Server::addOptions(options);

  SmartPointer<Option> opt;
  options.pushCategory("HTTP Server");

  opt = options.add("http-addresses", "A space separated list of server "
                    "address and port pairs to listen on in the form "
                    "<ip | hostname>[:<port>]");
  opt->setType(Option::TYPE_STRINGS);
  opt->setDefault("0.0.0.0:80");

  options.addTarget("http-max-body-size", maxBodySize,
                    "Maximum size of an HTTP request body.");
  options.addTarget("http-max-headers-size", maxHeaderSize,
                    "Maximum size of the HTTP request headers.");

  options.alias("connection-timeout", "http-timeout");
  options.alias("connection-backlog", "http-connection-backlog");
  options.alias("max-connections",    "http-max-connections");
  options.alias("max-ttl",            "http-max-ttl");

  options.popCategory();

  if (sslCtx.isSet()) {
    options.pushCategory("HTTP Server SSL");
    opt = options.add("https-addresses", "A space separated list of secure "
                      "server address and port pairs to listen on in the form "
                      "<ip | hostname>[:<port>]");
    opt->setType(Option::TYPE_STRINGS);
    opt->setDefault("0.0.0.0:443");
    options.add("crl-file", "Supply a Certificate Revocation List.  Overrides "
                "any internal CRL");
    options.add("certificate-file", "The servers certificate file in PEM "
                "format.")->setDefault("certificate.pem");
    options.add("private-key-file", "The servers private key file in PEM "
                "format.")->setDefault("private.pem");
    options.popCategory();
  }
}


void Server::init(Options &options) {
  Event::Server::init(options);

  // Configure ports
  auto addresses = options["http-addresses"].toStrings();
  for (auto &addr: addresses) addListenPort(SockAddr::parse(addr));

#ifdef HAVE_OPENSSL
  // SSL
  if (sslCtx.isSet()) {
    // Configure secure ports
    addresses = options["https-addresses"].toStrings();
    for (auto &addr: addresses) addSecureListenPort(SockAddr::parse(addr));

    // Load server certificate
    // TODO should load file relative to configuration file
    if (options["certificate-file"].hasValue()) {
      string certFile = options["certificate-file"].toString();
      if (SystemUtilities::exists(certFile))
        sslCtx->useCertificateChainFile(certFile);
      else LOG_WARNING("Certificate file not found " << certFile);
    }

    // Load server private key
    if (options["private-key-file"].hasValue()) {
      string priKeyFile = options["private-key-file"].toString();
      if (SystemUtilities::exists(priKeyFile))
        sslCtx->usePrivateKey(*SystemUtilities::open(priKeyFile));
      else LOG_WARNING("Private key file not found " << priKeyFile);
    }
  }
#endif // HAVE_OPENSSL
}


SmartPointer<Event::Connection> Server::createConnection() {
  auto conn = SmartPtr(new ConnIn(*this));
  conn->setMaxHeaderSize(maxHeaderSize);
  conn->setMaxBodySize(maxBodySize);
  return conn;
}


SmartPointer<Request> Server::createRequest(
  const SmartPointer<Conn> &connection, Method method, const URI &uri,
  const Version &version) {
  return new Request(connection, method, uri, version);
}


void Server::endRequest(Request &req) {
  if (logPrefix) Logger::instance().setPrefix("");
}


void Server::dispatch(Request &req) {
  RequestErrorHandler(*this)(req);
  TRY_CATCH_ERROR(endRequest(req));
}


bool Server::operator()(Request &req) {
  if (logPrefix) {
    string prefix = String::printf("REQ%" PRIu64 ":", req.getID());
    Logger::instance().setPrefix(prefix);
  }

  return HandlerGroup::operator()(req);
}
