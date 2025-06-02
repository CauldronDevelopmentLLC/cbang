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

#pragma once

#include "OptionMap.h"
#include "OptionCategory.h"

#include <cbang/json/Serializable.h>

#include <string>
#include <vector>
#include <map>


namespace cb {
  /// A container class for a set of configuration options
  class Options : public OptionMap, public JSON::Serializable {
    typedef std::map<const std::string, SmartPointer<Option> > map_t;
    map_t map;

    typedef std::map<const std::string, SmartPointer<OptionCategory> >
    categories_t;
    categories_t categories;

    typedef std::vector<SmartPointer<OptionCategory> > category_stack_t;
    category_stack_t categoryStack;

  public:
    static bool warnWhenInvalid;

    Options();

    typedef map_t::iterator iterator;
    typedef map_t::const_iterator const_iterator;

    bool empty() const {return map.empty();}
    iterator begin() {return map.begin();}
    iterator end() {return map.end();}
    const_iterator begin() const {return map.begin();}
    const_iterator end() const {return map.end();}

    void insert(JSON::Sink &sink, bool config = false) const;
    void write(JSON::Sink &sink, bool config) const;
    void write(XML::Handler &handler, uint32_t flags = 0) const;

    std::ostream &print(std::ostream &stream) const;
    void printHelp(std::ostream &stream, bool cmdLine = false) const;

    const SmartPointer<OptionCategory> &getCategory(const std::string &name);
    const SmartPointer<OptionCategory> &pushCategory(const std::string &name);
    void popCategory();

    void load(const JSON::Value &config);
    void dump(JSON::Sink &sink) const;

    JSON::ValuePtr getConfigJSON() const;

    // From OptionMap
    using OptionMap::add;
    void add(const std::string &name, SmartPointer<Option> option) override;
    bool remove(const std::string &key) override;
    bool has(const std::string &key) const override;
    const SmartPointer<Option> &get(const std::string &key) const override;
    void alias(const std::string &name, const std::string &alias) override;

    // From JSON::Serializable
    void read(const JSON::Value &value) override;
    void write(JSON::Sink &sink) const override {write(sink, false);}
    using JSON::Serializable::write;

    static std::string cleanKey(const std::string &key);
  };

  inline std::ostream &operator<<(std::ostream &stream, const Options &opts) {
    return opts.print(stream);
  }
};
