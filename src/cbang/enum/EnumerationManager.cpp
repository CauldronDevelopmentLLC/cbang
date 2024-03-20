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

#include "EnumerationManager.h"

#include <cbang/Exception.h>
#include <cbang/Application.h>
#include <cbang/String.h>

#include <cbang/config/CommandLine.h>

using namespace std;
using namespace cb;
using namespace cb;


EnumerationManager::EnumerationManager(Application &app) {
  if (!app.hasFeature(Application::FEATURE_ENUMERATION_MANAGER)) return;

  CommandLine &cmdLine = app.getCommandLine();

  typedef OptionAction<EnumerationManager> action_t;
  cb::SmartPointer<Option> opt;
  opt = cmdLine.add("enum", 0, new action_t(this, &EnumerationManager::action),
                    "Either list all available enumerations or the members of "
                    "an enumeration and exit.");
  opt->setType(Option::STRING_TYPE);
  opt->setOptional();
}


void EnumerationManager::print(ostream &stream, const string &name) const {
  auto it = enums.find(name);
  if (it == enums.end()) THROW("Enumeration '" << name << "' not found");

  for (unsigned i = 0; i < it->second.getCount(); i++)
    stream << it->second.getName(i) << '\n';
}


int EnumerationManager::action(Option &option) {
  if (option.hasValue()) print(cout, String::toLower(option.toString()));
  else for (auto &p: enums) cout << p.first << '\n';

  cout << flush;

  exit(0); // TODO Option calls the action so we have to exit(). FIXME!
  return -1;
}
