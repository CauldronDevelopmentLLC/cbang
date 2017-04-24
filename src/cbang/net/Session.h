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

#include <cbang/StdTypes.h>
#include <cbang/net/IPAddress.h>
#include <cbang/json/Serializable.h>
#include <cbang/time/Time.h>

#include <string>


namespace cb {
  class Session : public JSON::Serializable {
    std::string id;
    uint64_t creationTime;
    uint64_t lastUsed;

    std::string user;
    IPAddress ip;

  public:
    Session(const JSON::Value &value) {read(value);}
    Session(const std::string &id, const IPAddress &ip) :
      id(id), creationTime(Time::now()), lastUsed(Time::now()),
      ip(ip.getIP()) {}

    const std::string &getID() const {return id;}
    void setID(const std::string &id) {this->id = id;}

    void touch() {lastUsed = Time::now();}
    uint64_t getCreationTime() const {return creationTime;}
    void setCreationTime(uint64_t creationTime)
    {this->creationTime = creationTime;}

    uint64_t getLastUsed() const {return lastUsed;}
    void setLastUsed(uint64_t lastUsed) {this->lastUsed = lastUsed;}

    void setUser(const std::string &user) {this->user = user;}
    const std::string &getUser() const {return user;}

    const IPAddress &getIP() const {return ip;}
    void setIP(const IPAddress &ip) {this->ip = ip;}

    // From JSON::Serializable
    void read(const JSON::Value &value);
    void write(JSON::Sink &sink) const;
  };
}
