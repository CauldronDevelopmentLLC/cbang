/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "Session.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <map>


namespace cb {
  class Options;


  class SessionManager : public JSON::Serializable {
    typedef std::map<std::string, SmartPointer<Session> > sessions_t;
    sessions_t sessions;

    uint64_t lifetime    = Time::SEC_PER_DAY;
    uint64_t timeout     = Time::SEC_PER_HOUR;
    std::string cookie   = "sid";
    uint64_t lastCleanup = 0;

  public:
    SessionManager() {}
    SessionManager(Options &options) {addOptions(options);}

    void addOptions(Options &options);

    uint64_t getLifetime() const {return lifetime;}
    void setLifetime(uint64_t lifetime) {this->lifetime = lifetime;}

    uint64_t getTimeout() const {return timeout;}
    void setTimeout(uint64_t timeout) {this->timeout = timeout;}

    const std::string &getSessionCookie() const {return cookie;}
    void setSessionCookie(const std::string &cookie) {this->cookie = cookie;}

    std::string generateID(const SockAddr &addr);

    virtual bool isExpired(const Session &session) const;
    virtual bool hasSession(const std::string &sid) const;
    virtual SmartPointer<Session> lookupSession(const std::string &sid) const;
    virtual SmartPointer<Session> openSession(const SockAddr &addr);
    virtual void closeSession(const std::string &sid);
    virtual void addSession(const SmartPointer<Session> &session);
    virtual void cleanup();

    typedef sessions_t::const_iterator iterator;
    iterator begin() const {return sessions.begin();}
    iterator end() const {return sessions.end();}

    // From JSON::Serializable
    void read(const JSON::Value &value) override;
    void write(JSON::Sink &sink) const override;
  };
}
