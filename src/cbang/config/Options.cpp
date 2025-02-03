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


bool Options::warnWhenInvalid = false;


Options::Options() {
  pushCategory(""); // Default category
}


Options::~Options() {}


void Options::insert(JSON::Sink &sink, bool config) const {
  for (auto &p: categories)
    if (!p.second->getHidden() && !p.second->isEmpty() &&
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
  for (auto &p: categories) p.second->write(handler, flags);
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
    if (!p.second->getHidden()) {
      if (first) first = false;
      else stream << "\n\n";
      p.second->printHelp(stream, cmdLine);
    }
}


const SmartPointer<OptionCategory> &Options::getCategory(const string &name) {
  auto it = categories.find(name);

  if (it == categories.end())
    it = categories.insert(
      categories_t::value_type(name, new OptionCategory(name))).first;

  return it->second;
}


const SmartPointer<OptionCategory> &Options::pushCategory(const string &name) {
  const SmartPointer<OptionCategory> &category = getCategory(name);
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


void Options::add(const string &_key, SmartPointer<Option> option) {
  auto key = cleanKey(_key);
  auto it  = map.find(key);

  if (it != map.end()) THROW("Option '" << key << "' already exists.");

  map[key] = option;
  categoryStack.back()->add(option);
}


bool Options::remove(const string &key) {return map.erase(cleanKey(key));}


bool Options::has(const string &key) const {
  return map.find(cleanKey(key)) != map.end();
}


const SmartPointer<Option> &Options::get(const string &_key) const {
  string key = cleanKey(_key);

  auto it = map.find(key);

  if (it == map.end()) {
    if (getAutoAdd()) {
      const SmartPointer<Option> &option =
        const_cast<Options *>(this)->map[key] = new Option(key);
      categoryStack.back()->add(option);
      return option;

    } else THROW("Option '" << key << "' does not exist.");
  }

  return it->second;
}


void Options::alias(const string &_key, const string &_alias) {
  string key   = cleanKey(_key);
  string alias = cleanKey(_alias);

  const SmartPointer<Option> &option = localize(key);

  auto it = map.find(alias);
  if (it != map.end())
    THROW("Cannot alias, option '" << alias << "' already exists.");

  option->addAlias(alias);
  map[alias] = option;
}


void Options::read(const JSON::Value &options) {
  for (auto e: options.entries()) {
    string name = e.key();

    if (!has(name)) {
      LOG_WARNING("Unrecognized option '" << name << "'");
      continue;
    }

    if (e->isList()) {
      string s;
      for (auto &v: *e.value()) {
        if (!s.empty()) s += " ";
        s += v->asString();
      }
      set(name, s);

    } else set(name, e->asString());
  }
}


string Options::cleanKey(const string &key) {
  return String::replace(key, '_', '-');
}
