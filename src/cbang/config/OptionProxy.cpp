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

#include "OptionProxy.h"

#include <cbang/Exception.h>

using namespace std;
using namespace cb;


bool OptionProxy::has(const string &key) const {
  return local(key) || options.has(key);
}


bool OptionProxy::local(const string &key) const {
  return Options::has(key);
}


const SmartPointer<Option> &OptionProxy::localize(const string &key) {
  if (local(key)) return Options::get(key);

  // Create and add proxied Option
  SmartPointer<Option> option = new Option(options.get(key));
  Options::add(option);

  // Localize any aliases
  const string &name = option->getName(); // Get the non-aliased name
  for (auto &alias: option->getAliases())
    Options::alias(name, alias);

  return Options::get(key); // Get a const reference to the SmartPointer
}


const SmartPointer<Option> &OptionProxy::get(const string &key) const {
  if (local(key)) return Options::get(key);
  return options.get(key);
}
