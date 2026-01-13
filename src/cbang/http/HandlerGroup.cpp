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

#include "HandlerGroup.h"
#include "RE2PatternMatcher.h"
#include "MethodMatcher.h"
#include "ResourceHandler.h"
#include "IndexHandler.h"
#include "FileHandler.h"

using namespace cb::HTTP;
using namespace cb;
using namespace std;


void HandlerGroup::addHandler(
  const SmartPointer<RequestHandler> &handler) {handlers.push_back(handler);}


void HandlerGroup::addHandler(unsigned methods, const string &pattern,
                              const SmartPointer<RequestHandler> &handler) {
  addHandler(createMatcher(methods, prefix + pattern, handler));
}


void HandlerGroup::addHandler(const string &pattern, const Resource &res) {
  addHandler(HTTP_GET, pattern, createHandler(res));
}


void HandlerGroup::addHandler(const string &pattern, const string &path) {
  addHandler(HTTP_GET, pattern, createHandler(path));
}


SmartPointer<HandlerGroup> HandlerGroup::addGroup() {
  SmartPointer<HandlerGroup> group = new HandlerGroup;
  addHandler(group);
  return group;
}


SmartPointer<HandlerGroup> HandlerGroup::addGroup(
  unsigned methods, const string &pattern, const string &prefix) {
  SmartPointer<HandlerGroup> group = new HandlerGroup;
  group->setPrefix(this->prefix + prefix);
  addHandler(methods, pattern, group);
  return group;
}


bool HandlerGroup::operator()(Request &req) {
  for (auto &handler: handlers)
    if ((*handler)(req)) return true;

  return false;
}


SmartPointer<RequestHandler> HandlerGroup::createMatcher(
  unsigned methods, const string &pattern,
  const SmartPointer<RequestHandler> &child) {
  SmartPointer<RequestHandler> handler = child;

  if (!pattern.empty()) handler = new RE2PatternMatcher(pattern, handler);

  if (methods != (unsigned)Method::HTTP_ANY)
    handler = new MethodMatcher(methods, handler);

  return handler;
}


SmartPointer<RequestHandler> HandlerGroup::createHandler(
  const Resource &res) {
  SmartPointer<RequestHandler> handler = new ResourceHandler(res);
  return autoIndex ? new IndexHandler(handler) : handler;
}


SmartPointer<RequestHandler> HandlerGroup::createHandler(const string &path) {
  SmartPointer<RequestHandler> handler = new FileHandler(path);
  return autoIndex ? new IndexHandler(handler) : handler;
}
