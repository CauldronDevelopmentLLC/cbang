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

#include "BearerSessionManager.h"
#include "Status.h"

#include <cbang/config.h>
#include <cbang/String.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/time/Time.h>

#ifdef HAVE_OPENSSL
#include <cbang/net/Base64.h>
#include <cbang/openssl/Digest.h>
#include <cbang/openssl/KeyPair.h>
#endif

using namespace std;
using namespace cb;
using namespace cb::HTTP;


namespace {
  // Far longer than any token we issue
  const unsigned maxTokenLength = 8192;


  // The token of a ``Bearer <token>`` session ID.  The scheme is case
  // insensitive.
  bool getToken(const string &sid, string &token) {
    string s   = String::trim(sid);
    auto   end = s.find_first_of(String::DEFAULT_DELIMS);
    if (String::toLower(s.substr(0, end)) != "bearer") return false;

    token = end == string::npos ? "" : String::trim(s.substr(end));
    return true;
  }


  [[noreturn]] void invalid(const string &why) {
    THROWX("Invalid bearer token: " << why, Status::HTTP_UNAUTHORIZED);
  }


  // A time claim in seconds since the epoch, zero if absent
  uint64_t getTime(const JSON::Value &claims, const string &name) {
    if (!claims.has(name)) return 0;
    if (!claims.get(name)->isNumber()) invalid(name + " is not a number");
    return claims.getU64(name);
  }
}


BearerSessionManager::BearerSessionManager() {}
BearerSessionManager::~BearerSessionManager() {}


void BearerSessionManager::addPublicKeys(const string &pem) {
#ifdef HAVE_OPENSSL
  // One key per PEM block
  const string end = "-----END PUBLIC KEY-----";
  unsigned count = 0;
  size_t start = 0;

  while (true) {
    size_t pos = pem.find(end, start);
    if (pos == string::npos) break;
    pos += end.length();

    auto key = SmartPtr(new KeyPair);
    key->readPublicPEM(pem.substr(start, pos - start));
    if (!key->isRSA()) THROW("Bearer token keys must be RSA, for RS256");
    keys.push_back(key);

    start = pos;
    count++;
  }

  if (!count) THROW("No PEM public keys found");

#else
  THROW("Bearer tokens require OpenSSL");
#endif
}


void BearerSessionManager::readPublicKeys(const string &path) {
  addPublicKeys(SystemUtilities::read(path));
}


bool BearerSessionManager::hasSession(const string &sid) const {
  // Every bearer token has a session, or lookupSession() says why not.  So
  // no bearer token is ever treated as the ID of a stored session.
  string token;
  return getToken(sid, token) || SessionManager::hasSession(sid);
}


SmartPointer<Session>
BearerSessionManager::lookupSession(const string &sid) const {
  string token;
  if (!getToken(sid, token)) return SessionManager::lookupSession(sid);

  if (token.empty()) invalid("empty");
  if (maxTokenLength < token.length()) invalid("too long");

#ifdef HAVE_OPENSSL
  string id = Digest::hashHex(token, "sha256");

  auto it = cache.find(id);
  if (it != cache.end()) {
    if (!it->second.expires || Time::now() < it->second.expires)
      return it->second.session;
    cache.erase(it);
  }

  auto entry = verify(token);
  cache[id] = entry;
  return entry.session;

#else
  invalid("not supported without OpenSSL");
#endif
}


BearerSessionManager::Entry
BearerSessionManager::verify(const string &token) const {
#ifdef HAVE_OPENSSL
  // A JWS in compact form: header.claims.signature, where the signature may
  // be empty, as with "alg": "none"
  auto dot1 = token.find('.');
  auto dot2 = dot1 == string::npos ? dot1 : token.find('.', dot1 + 1);
  if (dot2 == string::npos || token.find('.', dot2 + 1) != string::npos)
    invalid("malformed");

  string signedData = token.substr(0, dot2);
  JSON::ValuePtr header;
  JSON::ValuePtr claims;
  string signature;

  try {
    URLBase64 base64;
    header    = JSON::Reader::parse(base64.decode(token.substr(0, dot1)));
    claims    = JSON::Reader::parse(
      base64.decode(token.substr(dot1 + 1, dot2 - dot1 - 1)));
    signature = base64.decode(token.substr(dot2 + 1));
  } catch (const Exception &) {invalid("malformed");}

  // Only RS256, so never "none" nor a public key used as an HMAC secret, and
  // no extensions
  if (!header->isDict() || !header->hasString("alg") ||
      header->getString("alg") != "RS256" || header->has("crit"))
    invalid("unsupported algorithm");

  string digest = Digest::hash(signedData, "sha256");
  bool verified = false;

  for (auto &key: keys)
    try {
      key->verify(signature, digest);
      verified = true;
      break;
    } catch (const Exception &) {}

  if (!verified) invalid("bad signature");

  if (!claims->isDict()) invalid("malformed claims");
  if (!claims->hasString("sub") || claims->getString("sub").empty())
    invalid("no subject");

  uint64_t now     = Time::now();
  uint64_t expires = getTime(*claims, "exp");
  if (expires && expires <= now) invalid("expired");
  if (now < getTime(*claims, "nbf")) invalid("not yet valid");

  auto session = SmartPtr(new Session);
  session->setID(Digest::hashHex(token, "sha256"));
  session->setUser(claims->getString("sub"));

  if (claims->has("groups")) {
    auto groups = claims->get("groups");
    if (!groups->isList()) invalid("groups is not a list");

    for (auto &group: *groups) {
      if (!group->isString()) invalid("a group is not a string");
      session->addGroup(group->getString());
    }
  }

  // Once per token and process, as verified sessions are cached
  LOG_INFO(1, "Verified bearer token for " << session->getUser());

  return {expires, session};

#else
  invalid("not supported without OpenSSL");
#endif
}
