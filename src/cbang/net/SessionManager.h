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

    uint64_t sessionLifetime;
    uint64_t sessionTimeout;
    std::string sessionCookie;

  public:
    SessionManager(Options &options);

    const std::string &getSessionCookie() const {return sessionCookie;}

    virtual bool hasSession(const std::string &sid) const;
    virtual SmartPointer<Session> lookupSession(const std::string &sid) const;
    virtual SmartPointer<Session> openSession(const std::string &user,
                                              const IPAddress &ip);
    virtual void closeSession(const std::string &sid);

    virtual void cleanup();

    typedef sessions_t::const_iterator iterator;
    iterator begin() const {return sessions.begin();}
    iterator end() const {return sessions.end();}

    // From JSON::Serializable
    void read(const JSON::Value &value);
    void write(JSON::Sink &sink) const;
  };
}
