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

#include "Options.h"
#include "OptionCategory.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/json/Dict.h>
#include <cbang/json/Sink.h>
#include <cbang/json/Builder.h>
#include <cbang/log/Logger.h>

#include <cctype>
#include <iomanip>

using namespace std;
using namespace cb;


Options::Options() {pushCategory("");} // Default category


void Options::add(OptionPtr option) {add(option->getName(), option);}


OptionPtr Options::add(const string &name, const string &help,
  const SmartPointer<Constraint> &constraint) {
  auto option = SmartPtr(new Option(name, help, constraint));
  add(option);
  return option;
}


OptionPtr Options::add(const string &name, const char shortName,
  SmartPointer<OptionActionBase> action, const string &help) {
  auto option = SmartPtr(new Option(name, shortName, action, help));
  add(option);
  return option;
}


void Options::set(Option &option, const JSON::ValuePtr &value) {
  if (!allowReset && !option.isPlural() && option.isSet())
    LOG_WARNING("Option '" << option.getName() << "' already set to '"
      << option << "' reseting to '" << value->asString() << "'.");

  option.set(value);
}


void Options::set(const string &name, const string &_value, bool setDefault) {
  if (autoAdd && !has(name)) add(new Option(name));

  auto option = tryLocalize(name);
  if (option.isNull()) return;

  auto value = option->parse(_value);
  if (setDefault) option->setDefault(value);
  else set(*option, value);
}


void Options::insert(JSON::Sink &sink, bool config) const {
  for (auto &p: categories)
    if (!p.second->isHidden() && !p.second->isEmpty() &&
        (!config || p.second->hasSetOption())) {
      if (!config) sink.beginInsert(p.first);
      p.second->write(sink, config);
    }
}


void Options::write(JSON::Sink &sink, bool config) const {
  sink.beginDict();
  insert(sink, config);
  sink.endDict();
}


void Options::write(XML::Handler &handler, uint32_t flags) const {
  for (auto &p: categories)
    p.second->write(handler, flags);
}


ostream &Options::print(ostream &stream) const {
  unsigned width = 30;

  // Determine max width
  for (auto &p: *this)
    if (!p.second->isHidden()) {
      unsigned len = p.second->getName().length();
      if (width < len) width = len;
    }

  // Print
  for (auto &p: *this)
    if (!p.second->isHidden()) {
      stream << setw(width) << p.second->getName() << " = ";

      if (p.second->hasValue()) stream << *p.second << '\n';
      else stream << "<undefined>" << '\n';
    }

  return stream;
}


void Options::printHelp(ostream &stream, bool cmdLine) const {
  bool first = true;

  for (auto &p: categories)
    if (!p.second->isHidden()) {
      if (first) first = false;
      else stream << "\n\n";
      p.second->printHelp(stream, cmdLine);
    }
}


const OptionCatPtr &Options::getCategory(const string &name) {
  auto it = categories.find(name);

  using value_type = decltype(categories)::value_type;
  if (it == categories.end())
    it = categories.insert(value_type(name, new OptionCategory(name))).first;

  return it->second;
}


const OptionCatPtr &Options::pushCategory(const string &name) {
  auto &category = getCategory(name);
  categoryStack.push_back(category);
  return category;
}


void Options::popCategory() {
  if (categoryStack.size() <= 1) THROW("Cannot pop category stack");
  categoryStack.pop_back();
}


void Options::load(const JSON::Value &config) {
  for (auto e: config.entries()) {
    auto category = e.key();
    if (!category.empty()) pushCategory(category);

    auto &opts = *e.value();
    for (auto e2: opts.entries()) {
      auto name    = cleanKey(e2.key());
      auto &config = *e2.value();

      if (has(name)) get(name)->configure(config);
      else add(name, new Option(name, config));

      if (config.hasList("aliases"))
        for (auto &a: config.getList("aliases"))
          alias(name, a->asString());
    }

    if (!category.empty()) popCategory();
  }
}


void Options::dump(JSON::Sink &sink) const {
  sink.beginDict();

  for (auto &cat: categories) {
    if (cat.second->isEmpty()) continue;

    sink.insertDict(cat.first);

    for (auto &o: *cat.second) {
      sink.beginInsert(o.first);
      o.second->dump(sink);
    }

    sink.endDict();
  }

  sink.endDict();
}


JSON::ValuePtr Options::getConfigJSON() const {
  JSON::Builder builder;
  write(builder, true);
  return builder.getRoot();
}


void Options::add(const string &_key, OptionPtr option) {
  auto key = cleanKey(_key);
  auto it  = map.find(key);

  if (it != map.end()) THROW("Option '" << key << "' already exists.");

  map[key] = option;
  categoryStack.back()->add(option);
}


bool Options::remove(const string &_key) {
  auto key = cleanKey(_key);

  for (auto &p: categories)
    p.second->remove(key);

  return map.erase(key);
}


bool Options::has(const string &key) const {
  return map.find(cleanKey(key)) != map.end();
}


const OptionPtr &Options::localize(const std::string &key) {return get(key);}


const OptionPtr &Options::get(const string &_key) const {
  string key = cleanKey(_key);

  auto it = map.find(key);
  if (it == map.end()) THROW("Option '" << key << "' does not exist.");

  return it->second;
}


void Options::alias(const string &_key, const string &_alias) {
  string key   = cleanKey(_key);
  string alias = cleanKey(_alias);

  const OptionPtr &option = localize(key);

  if (map.find(alias) != map.end())
    THROW("Cannot alias, option '" << alias << "' already exists.");

  option->addAlias(alias);
  map[alias] = option;
}


void Options::startElement(const string &name, const XML::Attributes &attrs) {
  setDefault =
    attrs.has("default") && String::parseBool(attrs["default"], true);

  auto it = attrs.find("v");
  if (it == attrs.end()) it = attrs.find("value");
  xmlValueSet = it != attrs.end();

  if (xmlValueSet) set(name, it->second, setDefault);

  xmlValue.clear();
}


void Options::endElement(const string &name) {
  xmlValue = String::trim(xmlValue);

  if (xmlValue.empty()) {
    // If value not set and type is boolean, set true
    if (!xmlValueSet && has(name)) {
      auto option = get(name);
      if (option->isBoolean())
        set(*option, JSON::Factory().createBoolean(true));
    }

  } else set(name, xmlValue, setDefault);
}


void Options::text (const string &text) {xmlValue.append(text);}
void Options::cdata(const string &data) {xmlValue.append(data);}


void Options::read(const JSON::Value &options) {
  for (auto e: options.entries()) {
    auto option = tryLocalize(e.key());
    if (option.isSet()) set(*option, e.value());
  }
}


string Options::cleanKey(const string &key) {
  return String::replace(key, '_', '-');
}


OptionPtr Options::tryLocalize(const string &name) {
  if (has(name)) return localize(name);
  LOG_WARNING("Unrecognized option '" << name << "'");
  return 0;
}
