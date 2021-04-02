/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
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

#include "HTTPHandlerGroup.h"

using namespace cb::Event;
using namespace cb;
using namespace std;


void HTTPHandlerGroup::addHandler
(const SmartPointer<HTTPRequestHandler> &handler) {handlers.push_back(handler);}


void HTTPHandlerGroup::addHandler
(unsigned methods, const string &pattern,
 const SmartPointer<HTTPRequestHandler> &handler) {
  addHandler(factory->createMatcher(methods, pattern, handler));
}


void HTTPHandlerGroup::addHandler(const string &search, const Resource &res) {
  addHandler(HTTP_GET, search, factory->createHandler(res));
}


void HTTPHandlerGroup::addHandler(const string &search, const string &path) {
  addHandler(HTTP_GET, search, factory->createHandler(path));
}


SmartPointer<HTTPHandlerGroup>
HTTPHandlerGroup::addGroup(unsigned methods, const string &search) {
  SmartPointer<HTTPHandlerGroup> group = new HTTPHandlerGroup(factory);
  addHandler(methods, search, group);
  return group;
}


bool HTTPHandlerGroup::operator()(Request &req) {
  for (unsigned i = 0; i < handlers.size(); i++)
    if ((*handlers[i])(req)) return true;

  return false;
}
