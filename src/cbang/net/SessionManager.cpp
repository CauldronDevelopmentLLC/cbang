/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2017, Cauldron Development LLC
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

#include "SessionManager.h"

#include <cbang/config/Options.h>
#include <cbang/util/Random.h>
#include <cbang/json/JSON.h>

#if HAVE_OPENSSL
#include <cbang/openssl/Digest.h>
#endif

#include <vector>

using namespace cb;
using namespace std;


SessionManager::SessionManager() :
  lifetime(Time::SEC_PER_DAY), timeout(Time::SEC_PER_HOUR), cookie("sid") {}


SessionManager::SessionManager(Options &options) :
  lifetime(Time::SEC_PER_DAY), timeout(Time::SEC_PER_HOUR), cookie("sid") {
  addOptions(options);
}


void SessionManager::addOptions(Options &options) {
  options.pushCategory("Session Management");

  options.addTarget("session-timeout", timeout, "The maximum time in seconds, "
                    "from the most recent usage, before a session timesout.  "
                    "Zero for no timeout.");
  options.addTarget("session-lifetime", lifetime, "The maximum session "
                    "lifetime in seconds.  Zero for unlimited lifetime.");
  options.addTarget("session-cookie", cookie, "The session cookie name.");

  options.popCategory();
}


string SessionManager::generateID(const IPAddress &ip) {
#ifdef HAVE_OPENSSL
  Digest digest("sha256");

  digest.update(ip.toString());
  digest.updateWith(Time::now());
  digest.updateWith(Random::instance().rand<uint64_t>());

  return digest.toBase64();
#else

  return SSTR("0x" << hex << Random::instance().rand<uint64_t>());
#endif
}


bool SessionManager::isExpired(const Session &session) const {
  uint64_t now = Time::now();
  return (timeout && session.getLastUsed() + timeout < now) ||
    (lifetime && session.getCreationTime() + lifetime < now);
}


bool SessionManager::hasSession(const string &sid) const {
  sessions_t::const_iterator it = sessions.find(sid);
  return it != sessions.end() && !isExpired(*it->second);
}


SmartPointer<Session> SessionManager::lookupSession(const string &sid) const {
  iterator it = sessions.find(sid);
  if (it == end()) THROWS("Session ID '" << sid << "' does not exist");
  if (isExpired(*it->second))
    THROWS("Session ID '" << sid << "' has expired, last_used="
           << Time(it->second->getLastUsed()).toString() << " created="
           << Time(it->second->getCreationTime()).toString() << " now="
           << Time().toString());

  it->second->touch(); // Update timestamp

  return it->second;
}


SmartPointer<Session> SessionManager::openSession(const IPAddress &ip) {
  // Create new session
  SmartPointer<Session> session = new Session(generateID(ip), ip);

  // Insert
  addSession(session);

  return session;
}


void SessionManager::closeSession(const string &sid) {sessions.erase(sid);}


void SessionManager::addSession(const SmartPointer<Session> &session) {
  pair<sessions_t::iterator, bool> result =
    sessions.insert(sessions_t::value_type(session->getID(), session));

  if (!result.second) result.first->second = session;
}


void SessionManager::cleanup() {
  // Remove expired Sessions
  for (sessions_t::iterator it = sessions.begin(); it != sessions.end();)
    if (isExpired(*it->second)) sessions.erase(it++);
    else it++;
}


void SessionManager::read(const JSON::Value &value) {
  for (unsigned i = 0; i < value.size(); i++) {
    SmartPointer<Session> session = new Session(*value.get(i));
    session->setID(value.keyAt(i));
    addSession(session);
  }
}


void SessionManager::write(JSON::Sink &sink) const {
  sink.beginDict();

  for (iterator it = begin(); it != end(); it++) {
    sink.beginInsert(it->first);
    it->second->write(sink);
  }

  sink.endDict();
}
