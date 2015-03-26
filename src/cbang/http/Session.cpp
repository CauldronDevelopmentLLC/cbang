/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#include <cbang/time/Time.h>
#include <cbang/json/Value.h>
#include <cbang/json/Sync.h>

using namespace cb::HTTP;


void Session::read(const cb::JSON::Value &value) {
  creationTime =
    value.has("created") ?
    (uint64_t)Time::parse(value.getString("created")) : 0;
  lastUsed =
    value.has("last_used") ?
    (uint64_t)Time::parse(value.getString("last_used")) : 0;
  user = value.getString("user", "");
  ip = value.has("ip") ? IPAddress(value.getString("ip")) : IPAddress();
}


void Session::write(cb::JSON::Sync &sync) const {
  sync.beginDict();
  sync.insert("created", Time(creationTime).toString());
  sync.insert("last_used", Time(lastUsed).toString());
  sync.insert("user", user);
  sync.insert("ip", ip.toString());
  sync.endDict();
}
