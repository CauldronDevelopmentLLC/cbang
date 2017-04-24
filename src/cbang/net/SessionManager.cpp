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


SessionManager::SessionManager(Options &options) :
  sessionLifetime(Time::SEC_PER_DAY), sessionTimeout(Time::SEC_PER_HOUR),
  sessionCookie("sid") {

  options.pushCategory("Session Management");

  options.addTarget("session-timeout", sessionTimeout, "The max maximum "
                    "amount of time in seconds since the last time a session "
                    "was used before it timesout.  Zero for no session "
                    "timeout.");
  options.addTarget("session-lifetime", sessionLifetime, "The maximum "
                    "session lifetime in seconds.  Zero for unlimited "
                    "session lifetime.");
  options.addTarget("session-cookie", sessionCookie, "The name of the "
                    "session cookie.");

  options.popCategory();
}


bool SessionManager::hasSession(const string &sid) const {
  return sessions.find(sid) != sessions.end();
}


SmartPointer<Session> SessionManager::lookupSession(const string &sid) const {
  iterator it = sessions.find(sid);
  if (it == end()) THROWS("Session ID '" << sid << "' does not exist");

  it->second->touch(); // Update timestamp

  return it->second;
}


SmartPointer<Session> SessionManager::openSession(const string &user,
                                                  const IPAddress &ip) {
  // Generate new session ID
  string sid;

#ifdef HAVE_OPENSSL
  Digest digest("sha256");

  digest.update(ip.toString());
  digest.updateWith(Time::now());
  digest.updateWith(Random::instance().rand<uint64_t>());

  sid = digest.toHexString();
#else

  sid = SSTR("0x" << hex << Random::instance().rand<uint64_t>());
#endif

  // Create new session
  SmartPointer<Session> session = new Session(sid, ip);

  // Insert
  sessions.insert(sessions_t::value_type(sid, session));

  return session;
}


void SessionManager::closeSession(const string &sid) {sessions.erase(sid);}


void SessionManager::cleanup() {
  uint64_t now = Time::now();

  // Remove expired Sessions
  vector<sessions_t::iterator> remove;

  for (sessions_t::iterator it = sessions.begin(); it != sessions.end(); it++)
    if ((sessionTimeout && it->second->getLastUsed() + sessionTimeout < now) ||
        (sessionLifetime &&
         it->second->getCreationTime() + sessionLifetime < now))
      remove.push_back(it);

  for (unsigned i = 0; i < remove.size(); i++)
    sessions.erase(remove[i]);
}


void SessionManager::read(const JSON::Value &value) {
  for (unsigned i = 0; i < value.size(); i++) {
    SmartPointer<Session> session = new Session(*value.get(i));

    pair<sessions_t::iterator, bool> result =
      sessions.insert(sessions_t::value_type(session->getID(), session));

    if (!result.second) result.first->second = session;
  }
}


void SessionManager::write(JSON::Sink &sink) const {
  sink.beginList();

  for (iterator it = begin(); it != end(); it++) {
    sink.beginAppend();
    it->second->write(sink);
  }

  sink.endList();
}
