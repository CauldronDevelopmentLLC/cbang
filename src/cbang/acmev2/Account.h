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

/// Based on https://tools.ietf.org/html/draft-ietf-acme-acme-09

#include "KeyCert.h"

#include <cbang/http/Client.h>
#include <cbang/http/Method.h>
#include <cbang/net/URI.h>
#include <cbang/openssl/KeyPair.h>
#include <cbang/json/Sink.h>
#include <cbang/json/Value.h>

#include <vector>
#include <functional>


namespace cb {
  class Options;
  namespace Event {class Event;}
  namespace HTTP {class HandlerGroup;}

  namespace ACMEv2 {
    static std::string letsencrypt_base =
      "https://acme-v02.api.letsencrypt.org/directory";
    static std::string letsencrypt_staging =
      "https://acme-staging-v02.api.letsencrypt.org/directory";

    class Account : public HTTP::Method::Enum {
      SmartPointer<RefCounted> lifetime;
      HTTP::Client &client;
      KeyPair key;

      std::string uriBase = letsencrypt_staging;
      std::string emails;

      double retryWait = 60 * 20;
      double renewPeriod = 15;

      typedef enum {
        STATE_IDLE,
        STATE_GET_DIR,
        STATE_REGISTER,
        STATE_NEW_ORDER,
        STATE_GET_AUTH,
        STATE_CHALLENGE,
        STATE_FINALIZE,
        STATE_GET_ORDER,
        STATE_GET_CERT,
      } state_t;
      state_t state = STATE_IDLE;

      unsigned currentKeyCert = 0;
      std::vector<SmartPointer<KeyCert> > keyCerts;

      JSON::ValuePtr directory;
      std::string nonce;
      std::string kid;

      std::string orderLink;
      JSON::ValuePtr order;
      unsigned currentAuth = 0;
      JSON::ValuePtr authorization;

      std::string challengeToken;

      cb::HTTP::Client::RequestPtr pr;

    public:
      typedef std::function<void (KeyCert &)> listener_t;

    protected:
      std::vector<listener_t> listeners;

      SmartPointer<Event::Event> retryEvent;

    public:
      Account(HTTP::Client &client);

      const KeyPair &getKey() const {return key;}
      void setKey(const KeyPair &key) {this->key = key;}

      void setURIBase(const std::string &uriBase) {this->uriBase = uriBase;}
      void setContactEmails(const std::string &emails) {this->emails = emails;}
      void setRetryWait(double retryWait) {this->retryWait = retryWait;}
      void setRenewPeriod(double renewPeriod) {this->renewPeriod = renewPeriod;}

      void addOptions(Options &options);
      void simpleInit(const KeyPair &key, const KeyPair &clientKey,
                      const std::string &domains,
                      const std::string &clientChain,
                      HTTP::HandlerGroup &group, listener_t cb,
                      unsigned updateRate = 60);
      void addListener(listener_t listener);
      void addHandler(HTTP::HandlerGroup &group);
      bool needsRenewal(const KeyCert &keyCert) const;
      unsigned certsReadyForRenewal() const;
      void add(const SmartPointer<KeyCert> &keyCert);
      void update();
      bool matchChallengePath(const std::string &path) const;

      KeyCert &getCurrentKeyCert();
      const KeyCert &getCurrentKeyCert() const;
      const std::vector<std::string> &getCurrentDomains() const;

      std::string getURL(const std::string &name) const;
      std::string getThumbprint() const;
      std::string getKeyAuthorization() const;

      void writeJWK(JSON::Sink &sink) const;
      std::string getProtected(const URI &uri) const;
      std::string getSignedRequest(const URI &uri,
                                   const std::string &payload) const;
      std::string getNewAcctPayload() const;
      std::string getNewOrderPayload() const;
      std::string getFinalizePayload() const;

      bool challengeRequest(HTTP::Request &req);

    protected:
      std::string getProblemString(const JSON::Value &problem) const;

      void call(const std::string &url, HTTP::Method method);
      void head(const std::string &url) {call(url, HTTP_HEAD);}
      void get(const std::string &url) {call(url, HTTP_GET);}
      void post(const std::string &url, const std::string &payload);

      void error(const std::string &msg, const JSON::Value &json);
      void nextKeyCert();
      void nextAuth();
      void next();
      void retry(HTTP::Request &req, double delay);
      void fail(double delay);
      void responseHandler(HTTP::Request &req);
    };
  }
}
