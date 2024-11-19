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

#include "Account.h"

#include <cbang/String.h>
#include <cbang/Catch.h>

#include <cbang/config/Options.h>
#include <cbang/net/Base64.h>
#include <cbang/log/Logger.h>
#include <cbang/json/BufferWriter.h>
#include <cbang/util/WeakCallback.h>

#include <cbang/time/Time.h>
#include <cbang/time/Timer.h>
#include <cbang/time/HumanTime.h>

#include <cbang/openssl/Digest.h>
#include <cbang/openssl/CSR.h>

#include <cbang/event/Base.h>
#include <cbang/event/Event.h>
#include <cbang/http/Request.h>
#include <cbang/http/HandlerGroup.h>

using namespace cb;
using namespace cb::ACMEv2;
using namespace std;


Account::Account(HTTP::Client &client) :
  client(client),
  retryEvent(client.getBase().newEvent(this, &Account::next)) {}


void Account::addOptions(Options &options) {
  options.pushCategory("ACME v2");
  options.addTarget("acmev2-base", uriBase, "The ACME v2 URI base.");
  options.addTarget("acmev2-emails", emails, "Space separated list of "
                    "certificate contact emails."
                    )->setType(Option::TYPE_STRINGS);
  options.addTarget("acmev2-retry-wait", retryWait, "The time in seconds to "
                    "wait between retries.");
  options.addTarget("acmev2-renewal-period", renewPeriod, "Renew certificates "
                    "this many days before expiration.");
  options.popCategory();
}


void Account::simpleInit(const KeyPair &key, const KeyPair &clientKey,
                         const string &domains, const string &clientChain,
                         HTTP::HandlerGroup &group, listener_t cb,
                         unsigned updateRate) {
  SmartPointer<ACMEv2::KeyCert> keyCert = new KeyCert(domains, clientKey);
  if (!clientChain.empty()) keyCert->getChain().parse(clientChain);

  if (cb) addListener(cb);
  setKey(key);
  addHandler(group);
  add(keyCert);

  // Check for renewals at updateRate starting now
  auto wcb = WeakCallback<RefCounted>(this, [this] () {update();});
  auto e = client.getBase().newEvent(wcb);
  e->add(updateRate);
  e->activate();
}


void Account::addListener(listener_t listener) {listeners.push_back(listener);}


void Account::addHandler(HTTP::HandlerGroup &group) {
  group.addMember(HTTP_GET, "^/\\.well-known/acme-challenge/.*", this,
    &Account::challengeRequest);
}


bool Account::needsRenewal(const KeyCert &keyCert) const {
  return keyCert.expiredIn(renewPeriod * Time::SEC_PER_DAY) &&
    keyCert.getWaitUntil() < Time::now();
}


unsigned Account::certsReadyForRenewal() const {
  unsigned count = 0;

  for (unsigned i = 0; i < keyCerts.size(); i++)
    if (needsRenewal(*keyCerts[i])) count++;

  return count;
}


void Account::add(const SmartPointer<KeyCert> &keyCert) {
  keyCerts.push_back(keyCert);
}


void Account::update() {
  if (state != STATE_IDLE || !certsReadyForRenewal()) return;

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

  return String::startsWith(path, prefix) &&
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
    // Insert order matters
    sink.insert("e", URLBase64().encode(key.getRSA_E().toBinString()));
    sink.insert("kty", "RSA");
    sink.insert("n", URLBase64().encode(key.getRSA_N().toBinString()));

  } else THROW("Unsupported key type");

  sink.endDict();
}


string Account::getProtected(const URI &uri) const {
  // TODO Implement ES256 (RFC7518 Section 3.1) and EdDSA var. Ed25519 (RFC8037)
  // signature algorithms.  See IETF ACME draft "Request Authentication".

  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insert("alg", "RS256");

  if (!kid.empty()) writer.insert("kid", kid);
  else {
    writer.beginInsert("jwk");
    writeJWK(writer);
  }

  if (nonce.empty()) THROW("Need nonce");
  writer.insert("nonce", nonce);
  writer.insert("url", uri.toString());
  writer.endDict();
  writer.flush();

  return writer.toString();
}


string Account::getSignedRequest(const URI &uri, const string &payload) const {
  string protected64 = URLBase64().encode(getProtected(uri));
  string payload64 = URLBase64().encode(payload);
  string signed64 =
    URLBase64().encode(key.signSHA256(protected64 + "." + payload64));

  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insert("protected", protected64);
  writer.insert("payload", payload64);
  writer.insert("signature", signed64);
  writer.endDict();
  writer.flush();

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
  writer.flush();

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
  writer.flush();

  return writer.toString();
}


string Account::getFinalizePayload() const {
  auto csr = getCurrentKeyCert().makeCSR();

  JSON::BufferWriter writer(0, true);

  writer.beginDict();
  writer.insert("csr", URLBase64().encode(csr->toDER()));
  writer.endDict();
  writer.flush();

  return writer.toString();
}


bool Account::challengeRequest(HTTP::Request &req) {
  if (matchChallengePath(req.getURI().getPath())) {
    req.reply(getKeyAuthorization());
    if (retryEvent->isPending()) retryEvent->activate();
    return true;
  }

  return false;
}


string Account::getProblemString(const JSON::Value &problem) const {
  string s = problem.getString("type");

  if (problem.hasDict("identifier")) {
    auto &id = *problem.get("identifier");
    s += " id: " + id.getString("type") + " " + id.getString("value");
  }

  s += " " + problem.getString("detail");

  if (problem.hasList("subproblems")) {
    auto &list = *problem.get("subproblems");
    for (unsigned i = 0; i < list.size(); i++)
      s += "\n  " + getProblemString(*list.get(i));
  }

  return s;
}


void Account::call(const string &url, HTTP::Method method) {
  HTTP::Client::callback_t cb =
    [this] (HTTP::Request &req) {responseHandler(req);};
  pr = client.call(getURL(url), method, WeakCall(this, cb));
  pr->send();
}


void Account::post(const string &url, const string &payload) {
  URI uri = getURL(url);
  string data = getSignedRequest(uri, payload);
  nonce.clear(); // Nonce used

  LOG_DEBUG(5, "Posting " << data);

  HTTP::Client::callback_t cb =
    [this] (HTTP::Request &req) {responseHandler(req);};
  pr = client.call(uri, HTTP_POST, data, WeakCall(this, cb));
  pr->getRequest()->outSet("Content-Type", "application/jose+json");
  pr->send();
}


void Account::error(const string &msg, const JSON::Value &json) {
  string err = json.hasDict("error") ?
    getProblemString(*json.get("error")) : json.toString();
  LOG_ERROR(msg << ": " << err);
  getCurrentKeyCert().setWaitUntil(Time::now() + retryWait);
}


void Account::nextKeyCert() {
  currentKeyCert++;
  state = STATE_NEW_ORDER;
  next();
}


void Account::nextAuth() {
  auto &auths = *order->get("authorizations");
  if (++currentAuth < auths.size()) state = STATE_GET_AUTH;
  else state = STATE_FINALIZE;
  next();
}


void Account::next() {
  if (STATE_GET_DIR < state && nonce.empty()) {
    head("newNonce");
    return;
  }

  switch (state) {
  case STATE_IDLE: break;
  case STATE_GET_DIR: get(uriBase); break;
  case STATE_REGISTER: post("newAccount", getNewAcctPayload()); break;

  case STATE_NEW_ORDER:
    for (; currentKeyCert < keyCerts.size(); currentKeyCert++)
      if (needsRenewal(getCurrentKeyCert())) {
        post("newOrder", getNewOrderPayload());
        return;
      }

    state = STATE_IDLE;
    break;

  case STATE_GET_AUTH: {
    auto &auths = *order->get("authorizations", new JSON::Dict);
    if (currentAuth < auths.size()) get(auths.getString(currentAuth));
    else nextKeyCert();
    break;
  }

  case STATE_CHALLENGE: {
    auto &challenges = *authorization->get("challenges");

    for (unsigned i = 0; i < challenges.size(); i++) {
      auto &challenge = *challenges.get(i);

      if (challenge.getString("type", "") == "http-01") {
        string uri = challenge.getString("url");
        challengeToken = challenge.getString("token");

        post(uri, "{}");
        break;
      }
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


void Account::retry(HTTP::Request &req, double delay) {
  // Check for rate limit
  if (req.inHas("Retry-After")) {
    string s = req.inGet("Retry-After");

    try {
      delay = String::parseDouble(s);
    } catch (...) {
      try {
        delay = Time::parse(s, Time::httpFormat) - Time::now();
      } catch (...) {
        LOG_ERROR("Failed to parse HTTP header Retry-After: " << s);
      }
    }

  }

  LOG_DEBUG(3, "Retrying certificate operation in " << HumanTime(delay));
  retryEvent->add(delay);
}


void Account::fail(double delay) {
  LOG_DEBUG(3, "Failed, retrying certificate in " << HumanTime(delay));
  getCurrentKeyCert().setWaitUntil(Time::now() + delay);
  nextKeyCert();
}



void Account::responseHandler(HTTP::Request &req) {
  if (!req.isOk()) LOG_ERROR(req.getInput());
  else try {
      LOG_DEBUG(5, "state=" << state << " response=" << req.getInput());

      if (req.inHas("Replay-Nonce")) nonce = req.inGet("Replay-Nonce");

      // Check if this was a nonce request
      if (req.getMethod() == HTTP_HEAD) {
        next();
        return;
      }

      if (req.inGet("Content-Type") == "application/problem+json") {
        LOG_WARNING("Account: " << getProblemString(*req.getInputJSON()));
        fail(retryWait);
        return;
      }

      if (req.getResponseCode() == HTTP::Status::HTTP_TOO_MANY_REQUESTS)
        return fail(Time::SEC_PER_HOUR);

      switch (state) {
      case STATE_IDLE: return;

      case STATE_GET_DIR: directory = req.getInputJSON(); break;
      case STATE_REGISTER: kid = req.inGet("Location"); break;

      case STATE_NEW_ORDER:
        orderLink = req.inGet("Location");
        order = req.getInputJSON();
        currentAuth = 0;
        break;

      case STATE_GET_AUTH: {
        authorization = req.getInputJSON();
        string status = authorization->getString("status");

        if (status == "pending") break;
        if (status == "processing") return retry(req, 5);
        if (status == "valid") return nextAuth();

        // status == invalid or revoked
        auto &challenges = *authorization->get("challenges");

        for (unsigned i = 0; i < challenges.size(); i++) {
          auto &ch = *challenges.get(i);

          if (ch.getString("type", "") == "http-01") {
            error("Failed to complete certificate challenge", ch);
            break;
          }
        }

        return nextKeyCert();
      }

      case STATE_CHALLENGE: {
        string status = req.getInputJSON()->getString("status");

        if (status == "valid") nextAuth();

        else if (status == "pending") {
          state = STATE_GET_AUTH;
          retry(req, 5);

        } else nextKeyCert();

        return;
      }

      case STATE_FINALIZE: state = STATE_GET_ORDER;
        // Fall through, finalize returns order

      case STATE_GET_ORDER: {
        order = req.getInputJSON();
        string status = order->getString("status");

        if (status == "processing") return retry(req, 5);
        if (status == "valid") break;

        error("Unexpected certificate order status", *order);
        return nextKeyCert();
      }

      case STATE_GET_CERT: {
        auto &keyCert = getCurrentKeyCert();
        auto &chain = keyCert.getChain();

        chain.clear();
        chain.parse(req.getInput());

        for (unsigned i = 0; i < listeners.size(); i++)
          TRY_CATCH_ERROR(listeners[i](keyCert));

        nextKeyCert();
        return;
      }
      }

      // Next state
      state = (state_t)(state + 1);
      next();
      return;

    } CATCH_ERROR;

  fail(retryWait);
}
