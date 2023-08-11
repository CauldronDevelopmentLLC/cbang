/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "OptionMap.h"

#include <cbang/String.h>

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;


void OptionMap::add(SmartPointer<Option> option) {
  add(option->getName(), option);
}


SmartPointer<Option>
OptionMap::add(const string &name, const string &help,
               const SmartPointer<Constraint> &constraint) {
  SmartPointer<Option> option = new Option(name, help, constraint);
  add(option);
  return option;
}


SmartPointer<Option> OptionMap::add(const string &name, const char shortName,
                                    SmartPointer<OptionActionBase> action,
                                    const string &help) {
  SmartPointer<Option> option = new Option(name, shortName, action, help);
  add(option);
  return option;
}


void OptionMap::set(const string &name, const string &value, bool setDefault) {
  if (!autoAdd && !has(name)) {
    LOG_WARNING("Unrecognized option '" << name << "'");
    return;
  }

  Option &option = *localize(name);
  if (fileTracker.hasFile()) option.setFilename(&fileTracker.getCurrentFile());

  if (setDefault) option.setDefault(value);
  else if (!allowReset && option.isPlural()) option.append(value);
  else {
    if (!allowReset && option.isSet())
      LOG_WARNING("Option '" << name << "' already set to '" << option
                  << "' reseting to '" << value << "'.");

    option.set(value);
  }
}


void OptionMap::startElement(const string &name, const XMLAttributes &attrs) {
  setDefault = attrs.has("default") && attrs["default"] == "true";

  XMLAttributes::const_iterator it = attrs.find("v");
  if (it == attrs.end()) it = attrs.find("value");
  xmlValueSet = it != attrs.end();

  if (xmlValueSet) set(name, it->second, setDefault);

  xmlValue = "";
}


void OptionMap::endElement(const string &name) {
  xmlValue = String::trim(xmlValue);

  if (xmlValue.empty()) {
    // If value not set and type is boolean, set true
    if (!xmlValueSet && has(name) &&
        get(name)->getType() == Option::BOOLEAN_TYPE) set(name, "true");

  } else set(name, xmlValue, setDefault);
}


void OptionMap::text(const string &text) {xmlValue.append(text);}
void OptionMap::cdata(const string &data) {xmlValue.append(data);}
