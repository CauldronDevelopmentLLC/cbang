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

#include "WebServer.h"
#include "Request.h"

#include <cbang/config.h>
#include <cbang/config/Options.h>
#include <cbang/log/Logger.h>
#include <cbang/config/Options.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/openssl/SSLContext.h>

using namespace std;
using namespace cb::Event;


WebServer::WebServer(cb::Options &options, cb::Event::Base &base,
                     const cb::SmartPointer<cb::SSLContext> &sslCtx,
                     const cb::SmartPointer<HTTPHandlerFactory> &factory) :
  HTTPHandlerGroup(factory), HTTPServer(base), options(options),
  sslCtx(sslCtx) {
  addOptions(options);
}


WebServer::~WebServer() {}


void WebServer::addOptions(cb::Options &options) {
  SmartPointer<Option> opt;
  options.pushCategory("HTTP Server");

  opt = options.add("http-addresses", "A space separated list of server "
                    "address and port pairs to listen on in the form "
                    "<ip | hostname>[:<port>]");
  opt->setType(Option::STRINGS_TYPE);
  opt->setDefault("0.0.0.0:80");

  options.add("allow", "Client addresses which are allowed to connect to this "
              "server.  This option overrides IPs which are denied in the "
              "deny option.  The pattern 0/0 matches all addresses."
              )->setDefault("0/0");
  options.add("deny", "Client address which are not allowed to connect to this "
              "server.")->setType(Option::STRINGS_TYPE);

  options.add("http-max-body-size", "Maximum size of an HTTP request body.");
  options.add("http-max-headers-size", "Maximum size of the HTTP request "
              "headers.");
  options.add("http-timeout", "Maximum time in seconds before an http "
              "request times out.  Zero indicates no timeout.");
  options.add("http-connection-backlog", "Size of the connection backlog "
              "queue.  Once this is full connections are rejected.");
  options.add("http-max-connections", "Maximum simultaneous HTTP connections "
              "per port");
  options.add("http-max-ttl", "Maximum HTTP client connection time in seconds");

  options.popCategory();

  if (sslCtx.isSet()) {
    options.pushCategory("HTTP Server SSL");
    opt = options.add("https-addresses", "A space separated list of secure "
                      "server address and port pairs to listen on in the form "
                      "<ip | hostname>[:<port>]");
    opt->setType(Option::STRINGS_TYPE);
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


void WebServer::init() {
  // IP filters
  ipFilter.allow(options["allow"]);
  ipFilter.deny(options["deny"]);

  // Configure HTTP
  if (options["http-max-body-size"].hasValue())
    setMaxBodySize(options["http-max-body-size"].toInteger());
  if (options["http-max-headers-size"].hasValue())
    setMaxHeaderSize(options["http-max-headers-size"].toInteger());
  if (options["http-timeout"].hasValue())
    setTimeout(options["http-timeout"].toInteger());
  if (options["http-connection-backlog"].hasValue())
    setConnectionBacklog(options["http-connection-backlog"].toInteger());
  if (options["http-max-connections"].hasValue())
    setMaxConnections(options["http-max-connections"].toInteger());
  if (options["http-max-ttl"].hasValue())
    setMaxConnectionTTL(options["http-max-ttl"].toInteger());

  // Configure ports
  Option::strings_t addresses = options["http-addresses"].toStrings();
  for (unsigned i = 0; i < addresses.size(); i++)
    addListenPort(addresses[i]);

#ifdef HAVE_OPENSSL
  // SSL
  if (sslCtx.isSet()) {
    // Configure secure ports
    addresses = options["https-addresses"].toStrings();
    for (unsigned i = 0; i < addresses.size(); i++)
      addSecureListenPort(addresses[i]);

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
        sslCtx->usePrivateKey(priKeyFile);
      else LOG_WARNING("Private key file not found " << priKeyFile);
    }
  }
#endif // HAVE_OPENSSL
}


bool WebServer::allow(Request &req) const {
  return ipFilter.isAllowed(req.getClientIP().getIP());
}


cb::SmartPointer<Request> WebServer::createRequest
(RequestMethod method, const cb::URI &uri, const cb::Version &version) {
  return new Request(method, uri, version);
}


bool WebServer::handleRequest(const cb::SmartPointer<Request> &req) {
  if (logPrefix) {
    string prefix = String::printf("REQ%" PRIu64 ":", req->getID());
    Logger::instance().setPrefix(prefix);
  }

  // TODO call allow() on the connection before handling any requests
  if (!allow(*req)) THROWX("Unauthorized", HTTP_UNAUTHORIZED);

  return HTTPHandlerGroup::operator()(*req);
}


void WebServer::endRequest(const cb::SmartPointer<Request> &req) {
  if (logPrefix) Logger::instance().setPrefix("");
}


void WebServer::allow(const string &spec) {
  LOG_INFO(5, "Allowing HTTP access to " << spec);
  ipFilter.allow(spec);
}


void WebServer::deny(const string &spec) {
  LOG_INFO(5, "Denying HTTP access to " << spec);
  ipFilter.deny(spec);
}


void WebServer::addListenPort(const cb::IPAddress &addr) {
  LOG_INFO(1, "Listening for HTTP on " << addr);
  bind(addr, 0, priority);
}


void WebServer::addSecureListenPort(const cb::IPAddress &addr) {
  LOG_INFO(1, "Listening for HTTPS on " << addr);
  bind(addr, sslCtx, priority);
}


void WebServer::setTimeout(int timeout) {
  setReadTimeout(timeout);
  setWriteTimeout(timeout);
}
