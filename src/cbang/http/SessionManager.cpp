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

#include "SessionManager.h"

#include "Request.h"

#include <cbang/config.h>
#include <cbang/config/Options.h>
#include <cbang/util/Random.h>
#include <cbang/net/Base64.h>
#include <cbang/json/JSON.h>

#include <vector>
#include <random>
#include <cstring>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


void SessionManager::addOptions(Options &options) {
  options.pushCategory("Session Management");
  options.addTarget("session-timeout", timeout, "The maximum time in seconds, "
                    "from the most recent usage, before a session timesout.  "
                    "Zero for no timeout.");
  options.addTarget("session-lifetime", lifetime, "The maximum session "
                    "lifetime in seconds.  Zero for unlimited lifetime.");
  options.addTarget("session-cookie", cookie, "The session cookie name.");
  options.addTarget("session-rotate-on-login", rotateOnLogin, "Give a "
                    "session a new ID when it logs in, so an ID known before "
                    "the login, e.g. one planted in the cookie, is worthless "
                    "after it.");
  options.addTarget("session-login-reply-id", loginReplyID, "Include the "
                    "session ID in the reply that starts an OAuth2 login, as "
                    "{id, redirect}.  Only for clients that send the ID "
                    "themselves rather than use the cookie.");
  options.popCategory();
}


string SessionManager::generateID(const SockAddr &addr) {
  // 256 bits from a CSPRNG.  With OpenSSL, Random is RAND_bytes().
  uint8_t bytes[32];

#ifdef HAVE_OPENSSL
  Random::instance().bytes(bytes, sizeof(bytes));
#else
  // Without OpenSSL Random is rand(), which is predictable
  random_device rd;
  for (unsigned i = 0; i < sizeof(bytes); i += 4) {
    uint32_t x = rd();
    memcpy(bytes + i, &x, 4);
  }
#endif

  return URLBase64().encode((const char *)bytes, sizeof(bytes));
}


void SessionManager::sendCookie(Request &req, const string &sid) const {
  req.setCookie(cookie, sid, "", "/", 0, 0, cookieHTTPOnly, cookieSecure,
                cookieSameSite);
}


string SessionManager::rotateSession(const SmartPointer<Session> &session) {
  string oldID = session->getID();

  // Drop the old ID only if it is this session's, never another's
  auto it = sessions.find(oldID);
  if (it != sessions.end() && it->second.get() == session.get())
    sessions.erase(it);

  session->setID(generateID(SockAddr()));
  sessions[session->getID()] = session;

  return oldID;
}


bool SessionManager::isExpired(const Session &session) const {
  uint64_t now = Time::now();
  uint64_t timeout = session.getU64("timeout", this->timeout);
  uint64_t lifetime = session.getU64("lifetime", this->lifetime);

  return (timeout && session.getLastUsed() + timeout < now) ||
    (lifetime && session.getCreationTime() + lifetime < now);
}


bool SessionManager::hasSession(const string &sid) const {
  auto it = sessions.find(sid);
  return it != sessions.end() && !isExpired(*it->second);
}


SmartPointer<Session> SessionManager::lookupSession(const string &sid) const {
  auto it = sessions.find(sid);
  if (it == end() || isExpired(*it->second))
    THROW("Session ID '" << sid << "' does not exist");

  it->second->touch(); // Update timestamp

  return it->second;
}


SmartPointer<Session> SessionManager::openSession(const SockAddr &addr) {
  // Create new session
  SmartPointer<Session> session = new Session(generateID(addr), addr);

  // Insert
  addSession(session);

  return session;
}


void SessionManager::closeSession(const string &sid) {sessions.erase(sid);}


void SessionManager::addSession(const SmartPointer<Session> &session) {
  if (isExpired(*session)) return;

  auto result =
    sessions.insert(sessions_t::value_type(session->getID(), session));

  if (!result.second) result.first->second = session;

  if (lastCleanup + Time::SEC_PER_HOUR < Time::now()) cleanup();
}


void SessionManager::cleanup() {
  lastCleanup = Time::now();

  // Remove expired Sessions
  for (auto it = sessions.begin(); it != sessions.end();)
    if (isExpired(*it->second)) it = sessions.erase(it);
    else it++;
}


void SessionManager::read(const JSON::Value &value) {
  for (auto e: value.entries()) {
    auto session = SmartPtr(new Session(*e.value()));
    session->setID(e.key());
    addSession(session);
  }
}


void SessionManager::write(JSON::Sink &sink) const {
  sink.beginDict();

  for (auto &p: *this) {
    if (isExpired(*p.second)) continue;
    sink.beginInsert(p.first);
    p.second->write(sink);
  }

  sink.endDict();
}
