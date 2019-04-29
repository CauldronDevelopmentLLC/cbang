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

#include "Account.h"

#include <cbang/String.h>
#include <cbang/Catch.h>

#include <cbang/config/Options.h>
#include <cbang/net/Base64.h>
#include <cbang/log/Logger.h>
#include <cbang/json/BufferWriter.h>

#include <cbang/openssl/Digest.h>
#include <cbang/openssl/KeyContext.h>
#include <cbang/openssl/CSR.h>

#include <cbang/event/Base.h>
#include <cbang/event/Event.h>
#include <cbang/event/OutgoingRequest.h>

using namespace cb;
using namespace cb::ACMEv2;
using namespace std;


Account::Account(Event::Client &client, const KeyPair &key) :
  client(client), key(key),
  uriBase("https://acme-staging-v02.api.letsencrypt.org"), retryWait(5),
  maxRetries(5), renewPeriod(15), state(STATE_IDLE), retries(0),
  currentKeyCert(0), currentAuth(0) {}


void Account::addOptions(Options &options) {
  options.pushCategory("ACME v2");
  options.addTarget("acmev2-base-uri", uriBase, "The ACME v2 URI base.");
  options.addTarget("acmev2-contact-emails", emails, "Space separated list of "
                    "certificate contact emails."
                    )->setType(Option::STRINGS_TYPE);
  options.addTarget("acmev2-max-retries", maxRetries, "Maximum number of times "
                    "to retry an operation before giving up.");
  options.addTarget("acmev2-retry-wait", retryWait, "The time in seconds to "
                    "wait between retries.");
  options.addTarget("acmev2-renewal-period", renewPeriod, "Renew certificates "
                    "this many days before expiration.");
  options.addTarget("acmev2-cert-org", certOrg, "Certificate organization.");
  options.addTarget("acmev2-cert-unit", certUnit,
                    "Certificate organizational unit.");
  options.addTarget("acmev2-cert-location", certLocation,
                    "Certificate city or town.");
  options.addTarget("acmev2-cert-state", certState,
                    "Certificate state or province.");
  options.addTarget("acmev2-cert-country", certCountry,
                    "Certificate two-letter ISO country code.");
  options.popCategory();
}


void Account::addKeyCert(const SmartPointer<KeyCert> &keyCert) {
  keyCerts.push_back(keyCert);
}


void Account::update() {
  if (state != STATE_IDLE) return;

  if (directory.isNull()) state = STATE_GET_DIR;
  else if (kid.empty()) state = STATE_REGISTER;
  else {
    currentKeyCert = 0;
    state = STATE_NEW_ORDER;
  }

  try {
    nonce.clear(); // Nonce stale after idle
    next();
  } CATCH_ERROR;
}


bool Account::matchChallengePath(const string &path) const {
  const string prefix = "/.well-known/acme-challenge/";

  return state == STATE_CHALLENGE && String::startsWith(path, prefix) &&
    challengeToken == path.substr(prefix.length());
}


KeyCert &Account::getCurrentKeyCert() {
  if (keyCerts.empty()) THROW("Not active KeyCerts");
  return *keyCerts.at(currentKeyCert);
}


const KeyCert &Account::getCurrentKeyCert() const {
  return const_cast<Account *>(this)->getCurrentKeyCert();
}


const vector<string> &Account::getCurrentDomains() const {
  return getCurrentKeyCert().getDomains();
}


string Account::getURL(const string &url) const {
  if (directory.isNull() || String::startsWith(url, "http")) return url;
  return directory->getString(url);
}


string Account::getThumbprint() const {
  string json = JSON::BufferWriter(0, true).toString(this, &Account::writeJWK);
  return URLBase64().encode(Digest::hash(json, "sha256"));
}


string Account::getKeyAuthorization() const {
  return challengeToken + "." + getThumbprint();
}


void Account::writeJWK(JSON::Sink &sink) const {
  sink.beginDict();

  if (key.isRSA()) {
    sink.insert("kty", "RSA");
    sink.insert("e", URLBase64().encode(key.getRSA().getE().toBinString()));
    sink.insert("n", URLBase64().encode(key.getRSA().getN().toBinString()));

  } else THROW("Unsupported key type");

  sink.endDict();
}


string Account::getProtected(const URI &uri) const {
  // TODO Implement ES256 (RFC7518 Section 3.1) and EdDSA var. Ed25519 (RFC8037)
  // signature algorithms.  See IETF ACME draft "Request Authentication".

  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insert("alg", "RS256");

  if (kid.empty()) writer.insert("kid", kid);
  else {
    writer.beginInsert("jwk");
    writeJWK(writer);
  }

  if (nonce.empty()) THROW("Need nonce");
  writer.insert("nonce", nonce);
  writer.insert("url", uri.toString());
  writer.endDict();

  return writer.toString();
}


string Account::getSignedRequest(const URI &uri, const string &payload) const {
  string protected64 = URLBase64().encode(getProtected(uri));
  string payload64 = URLBase64().encode(payload);

  KeyContext ctx(key);
  ctx.setSignatureMD("sha256");
  string signed64 = URLBase64().encode(ctx.sign(protected64 + "." + payload64));

  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insert("protected", protected64);
  writer.insert("payload", payload64);
  writer.insert("signature", signed64);
  writer.endDict();

  return writer.toString();
}


string Account::getNewAcctPayload() const {
  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insertBoolean("termsOfServiceAgreed", true);

  if (!emails.empty()) {
    vector<string> list;
    String::tokenize(emails, list);

    writer.insertList("contact");
    for (unsigned i = 0; i < list.size(); i++)
      writer.append("mailto:" + list[i]);
    writer.endList();
  }

  writer.endDict();

  return writer.toString();
}


string Account::getNewOrderPayload() const {
  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insertList("identifiers");

  const vector<string> &domains = getCurrentDomains();
  for (unsigned i = 0; i < domains.size(); i++) {
    writer.appendDict();
    writer.insert("type", "dns");
    writer.insert("value", domains[i]);
    writer.endDict();
  }

  writer.endList();
  writer.endDict();

  return writer.toString();
}


string Account::getCheckChallengePayload() const {
  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insert("resource", "challenge");
  writer.insert("keyAuthorization", getKeyAuthorization());
  writer.endDict();

  return writer.toString();
}


string Account::getFinalizePayload() const {
  CSR csr;
  const vector<string> &domains = getCurrentDomains();

  if (domains.empty()) THROW("No domains set");

  csr.addNameEntry("CN", domains[0]);

  if (!certOrg.empty())      csr.addNameEntry("O",  certOrg);
  if (!certUnit.empty())     csr.addNameEntry("OU", certUnit);
  if (!certLocation.empty()) csr.addNameEntry("L",  certLocation);
  if (!certState.empty())    csr.addNameEntry("ST", certState);
  if (!certCountry.empty())  csr.addNameEntry("C",  certCountry);

  if (1 < domains.size()) {
    string subjectAltName;

    for (unsigned i = 1; i < domains.size(); i++) {
      if (1 < i) subjectAltName += ", ";
      subjectAltName += "DNS:" + domains[i];
    }

    csr.addExtension("subjectAltName", subjectAltName);
  }

  csr.sign(getCurrentKeyCert().getKey(), "sha256");

  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insert("csr", URLBase64().encode(csr.toDER()));
  writer.endDict();

  return writer.toString();
}


string Account::getProblemString(const JSON::ValuePtr &problem) const {
  string s = problem->getString("type");

  if (problem->hasDict("identifier")) {
    JSON::ValuePtr _id = problem->get("identifier");
    s += " id: " + _id->getString("type") + " " + _id->getString("value");
  }

  s += " " + problem->getString("detail");

  if (problem->hasList("subproblems")) {
    JSON::ValuePtr list = problem->get("subproblems");
    for (unsigned i = 0; i < list->size(); i++)
      s += "\n  " + getProblemString(list->get(i));
  }

  return s;
}


void Account::call(const string &url, Event::RequestMethod method) {
  client.call(getURL(url), method, this, &Account::responseHandler)->send();
}


void Account::post(const string &url, const string &payload) {
  URI uri = getURL(url);
  string data = getSignedRequest(uri, payload);
  nonce.clear(); // Nonce used

  SmartPointer<Event::OutgoingRequest> pr =
    client.call(uri, HTTP_POST, data, this, &Account::responseHandler);

  pr->outSet("Content-Type", "application/jose+json");
  pr->send();
}


void Account::nextKeyCert() {
  currentKeyCert++;
  state = STATE_NEW_ORDER;
  next();
}


void Account::nextAuth() {
  currentAuth++;
  state = STATE_GET_AUTH;
  next();
}


void Account::next() {
  retries = 0; // Reset retry count

  if (STATE_GET_DIR < state && nonce.empty()) {
    head("newNonce");
    return;
  }

  switch (state) {
  case STATE_IDLE: break;
  case STATE_GET_DIR: get(uriBase + "/directory"); break;
  case STATE_REGISTER: post("newAccount", getNewAcctPayload()); break;

  case STATE_NEW_ORDER:
    for (; currentKeyCert < keyCerts.size(); currentKeyCert++)
      if (getCurrentKeyCert().expiredIn(renewPeriod * Time::SEC_PER_DAY)) {
        post("newOrder", getNewOrderPayload());
        return;
      }

    state = STATE_IDLE;
    break;

  case STATE_GET_AUTH: {
    JSON::ValuePtr authz = order->get("authorizations");
    if (currentAuth < authz->size()) get(authz->getString(currentAuth));
    else nextKeyCert();
    break;
  }

  case STATE_CHALLENGE: {
    JSON::ValuePtr challenges = order->get("challenges");

    for (unsigned i = 0; i < challenges->size(); i++)
      if (challenges->get(i)->getString("type", "") == "http-01") {
        JSON::ValuePtr challenge = challenges->get(i);
        string uri = challenge->getString("uri");
        challengeToken = challenge->getString("token");

        post(uri, getCheckChallengePayload());
        break;
      }

    break;
  }

  case STATE_FINALIZE:
    post(order->getString("finalize"), getFinalizePayload());
    break;

  case STATE_GET_ORDER: post(orderLink, ""); break;
  case STATE_GET_CERT: post(order->getString("certificate"), ""); break;
  }
}


void Account::retry() {
  if (retries++ < maxRetries)
    client.getBase().newEvent(this, &Account::next)->add(retryWait);

  else
    switch (state) {
    case STATE_NEW_ORDER:
    case STATE_GET_AUTH:
      nextKeyCert();
      break;

    case STATE_CHALLENGE:
    case STATE_FINALIZE:
    case STATE_GET_ORDER:
    case STATE_GET_CERT:
      nextAuth();
      break;

    default: state = STATE_IDLE; break; // Give up
  }
}



void Account::responseHandler(Event::Request &req) {
  if (req.isOk())
    try {
      if (req.inHas("Replay-Nonce")) nonce = req.inGet("Replay-Nonce");

      // Check if this was a nonce request
      if (req.getMethod() == HTTP_HEAD) {
        next();
        return;
      }

      if (req.inGet("Content-Type") == "application/problem+json") {
        LOG_WARNING("Account: " << getProblemString(req.getInputJSON()));
        // TODO handle rate limits and Retry-After header
        retry();
        return;
      }

      switch (state) {
      case STATE_IDLE: return;

      case STATE_GET_DIR: directory = req.getInputJSON(); break;
      case STATE_REGISTER: kid = req.inGet("Location"); break;

      case STATE_NEW_ORDER:
        orderLink = req.inGet("Location");
        order = req.getInputJSON();
        currentAuth = 0;
        break;

      case STATE_GET_AUTH: authorization = req.getInputJSON(); break;

      case STATE_CHALLENGE: {
        string status = req.getInputJSON()->getString("status");

        if (status == "valid") break;
        else if (status == "pending") retry();
        else nextAuth();
        return;
      }

      case STATE_FINALIZE: state = STATE_GET_ORDER;
        // Fall through, finalize returns order

      case STATE_GET_ORDER: {
        order = req.getInputJSON();
        string status = order->getString("status");

        if (status == "processing") {
          retry();
          return;

        } else if (status != "valid") {
          nextAuth();
          return;
        }

        break;
      }

      case STATE_GET_CERT: {
        getCurrentKeyCert().updateChain(req.getInput());
        nextKeyCert();
        return;
      }
      }

      // Next state
      state = (state_t)(state + 1);
      next();
      return;

    } CATCH_ERROR;

  retry();
}
