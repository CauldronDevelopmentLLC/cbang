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

#include "Providers.h"
#include "GoogleProvider.h"
#include "GitHubProvider.h"
#include "FacebookProvider.h"

using namespace std;
using namespace cb;
using namespace cb::OAuth2;


void Providers::add(
  const string &name, const SmartPointer<Provider> &provider) {
  if (!providers.insert(providers_t::value_type(name, provider)).second)
    THROW("Already have provider: " << name);
}


bool Providers::has(const string &name) const {
  return providers.find(name) != end();
}


SmartPointer<Provider> Providers::get(const string &name) const {
  auto it = providers.find(name);
  return it == end() ? 0 : it->second;
}


void Providers::load(const string &provider) {
  if      (provider == "google")   add(provider, new GoogleProvider);
  else if (provider == "github")   add(provider, new GitHubProvider);
  else if (provider == "facebook") add(provider, new FacebookProvider);
  else THROW("Unknown OAuth2 provider: " << provider);
}


void Providers::loadAll() {
  load("google");
  load("github");
  load("facebook");
}


void Providers::addOptions(Options &options) {
  for (auto &p: providers)
    p.second->addOptions(options);
}
