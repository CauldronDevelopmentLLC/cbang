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

#include "Session.h"

#include <cbang/json/JSON.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


Session::Session() {if (!hasDict("group")) insertDict("group");}


Session::Session(const JSON::Value &value) {
  read(value);
  if (!hasDict("group")) insertDict("group");
}


Session::Session(const string &id, const SockAddr &addr) {
  setID(id);
  setCreationTime(Time::now());
  setLastUsed(Time::now());
  setAddr(addr);
  insertDict("group");
}


uint64_t Session::getCreationTime() const {
  return Time::parse(getString("created"));
}


void Session::setCreationTime(uint64_t creationTime) {
  insert("created", Time(creationTime).toString());
}


uint64_t Session::getLastUsed() const {
  return Time::parse(getString("last_used"));
}


void Session::setLastUsed(uint64_t lastUsed) {
  insert("last_used", Time(lastUsed).toString());
}


void Session::matchAddr(const SockAddr &addr) const {
  if (addr != getAddr())
    THROW("Session address changed from " << getAddr() << " to " << addr);
}


bool Session::hasGroup(const string &group) const {
  return get("group")->getBoolean(group, false);
}


void Session::addGroup(const string &group) {
  get("group")->insertBoolean(group, true);
}


vector<string> Session::getGroups() const {
  vector<string> groups;

  auto g = get("group");
  for (auto e: g->entries())
    if (e.value()->getBoolean()) groups.push_back(e.key());

  return groups;
}


void Session::read(const JSON::Value &value) {
  for (auto it = value.begin(); it != value.end(); it++)
    insert(it.key(), *it);
}
