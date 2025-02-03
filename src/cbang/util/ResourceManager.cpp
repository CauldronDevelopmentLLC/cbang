/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "ResourceManager.h"

#include <cbang/log/Logger.h>
#include <cbang/io/ArrayStream.h>

using namespace std;
using namespace cb;


ResourceManager *ResourceManager::singleton = 0;


ResourceManager &ResourceManager::instance() {
  if (!singleton) singleton = new ResourceManager;
  return *singleton;
}


void ResourceManager::add(const string &ns, const Resource *res) {
  if (!resources.insert(resources_t::value_type(ns, res)).second)
    THROW("Resource with namespace '" << ns << "' already exists");
}


const Resource *ResourceManager::find(const string &path) const {
  auto slash = path.find('/');
  string ns = slash == string::npos ? path : path.substr(0, slash);

  auto it = resources.find(ns);
  if (it == resources.end()) return 0;

  if (slash == string::npos) return it->second;

  return it->second->find(path.substr(slash));
}


const Resource &ResourceManager::get(const string &path) const {
  auto res = find(path);
  if (!res) THROW("Resource '" << path << "' not found");
  return *res;
}


SmartPointer<istream> ResourceManager::open(const string &path) const {
  auto &res = get(path);
  return new ArrayStream<const char>(res.getData(), res.getLength());
}
