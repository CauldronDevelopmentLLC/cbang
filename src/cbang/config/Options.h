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

#include "Option.h"
#include "OptionActionSet.h"
#include "OptionCategory.h"

#include <cbang/enum/Enumeration.h>
#include <cbang/xml/HandlerFactory.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Serializable.h>

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <type_traits>


namespace cb {
  /// A container class for a set of configuration options
  class Options : public XML::Handler, public JSON::Serializable {
    std::string xmlValue;
    bool xmlValueSet = false;
    bool setDefault  = false;
    bool autoAdd     = false;
    bool allowReset  = false;

    std::map<const std::string, OptionPtr> map;
    std::map<const std::string, OptionCatPtr> categories;
    std::vector<OptionCatPtr> categoryStack;

  public:
    Options();

    bool empty() const {return map.empty();}

    using iterator = decltype(map)::iterator;
    iterator begin() {return map.begin();}
    iterator end()   {return map.end();}

    using const_iterator = decltype(map)::const_iterator;
    const_iterator begin() const {return map.begin();}
    const_iterator end()   const {return map.end();}

    bool getAutoAdd() const {return autoAdd;}
    void setAutoAdd(bool x) {autoAdd = x;}
    bool getAllowReset() const {return allowReset;}
    void setAllowReset(bool x) {allowReset = x;}

    void add(OptionPtr option);
    OptionPtr add(const std::string &name, const std::string &help,
      const SmartPointer<Constraint> &constraint = 0);
    OptionPtr add(const std::string &name, const char shortName = 0,
      SmartPointer<OptionActionBase> action = 0, const std::string &help = "");

    template <typename T>
    OptionPtr add(const std::string &name, const char shortName,
      T *obj, typename OptionAction<T>::member_t member,
      const std::string &help = "") {
      return add(name, shortName, new OptionAction<T>(obj, member), help);
    }

    template <typename T>
    OptionPtr add(const std::string &name, const char shortName,
      T *obj, typename BareOptionAction<T>::member_t member,
      const std::string &help = "") {
      return add(name, shortName, new BareOptionAction<T>(obj, member), help);
    }

    template <typename T>
    void bind(const std::string &name, T &target) {
      auto action = SmartPtr(new OptionActionSet<T>(target));
      auto option = get(name);
      option->setAction(action);
      option->setDefaultSetAction(action);
      if (option->hasValue()) (*action)(*option);
    }

    template <typename T>
    typename std::enable_if<
      std::is_base_of<EnumerationBase, T>::value, void>::type
    setOptionDefault(Option &option, T &value) {
      option.setDefault(value.toString());
    }

    template <typename T>
    typename std::enable_if<
      !std::is_base_of<EnumerationBase, T>::value, void>::type
    setOptionDefault(Option &option, T &value) {option.setDefault(value);}

    template <typename T>
    OptionPtr addTarget(
      const std::string &name, T &target, const std::string &help = "",
      char shortName = 0) {
      auto option = add(name, shortName, 0, help);
      bind(name, target);
      setOptionDefault(*option, target);
      return option;
    }

    Option &operator[](const std::string &key) const {return *get(key);}

    void set(Option &option, const JSON::ValuePtr &value);
    void set(const std::string &name, const std::string &value,
      bool setDefault = false);

    void insert(JSON::Sink &sink, bool config = false) const;
    void write(JSON::Sink &sink, bool config) const;
    void write(XML::Handler &handler, uint32_t flags = 0) const;

    std::ostream &print(std::ostream &stream) const;
    void printHelp(std::ostream &stream, bool cmdLine = false) const;

    const OptionCatPtr &getCategory (const std::string &name);
    const OptionCatPtr &pushCategory(const std::string &name);
    void popCategory();

    void load(const JSON::Value &config);
    void dump(JSON::Sink &sink) const;

    JSON::ValuePtr getConfigJSON() const;

    // Virtual interface
    virtual void add(const std::string &name, OptionPtr option);
    virtual bool remove(const std::string &key);
    virtual bool has(const std::string &key) const;
    virtual bool local(const std::string &key) const {return true;}
    virtual const OptionPtr &localize(const std::string &key);
    virtual const OptionPtr &get(const std::string &key) const;
    virtual void alias(const std::string &name, const std::string &alias);

    // From XML::Handler
    void startElement(
      const std::string &name, const XML::Attributes &attrs) override;
    void endElement(const std::string &name) override;
    void text(const std::string &text) override;
    void cdata(const std::string &data) override;

    // From JSON::Serializable
    void read(const JSON::Value &value) override;
    void write(JSON::Sink &sink) const override {write(sink, false);}
    using JSON::Serializable::write;

    static std::string cleanKey(const std::string &key);

  protected:
    OptionPtr tryLocalize(const std::string &name);
  };


  inline std::ostream &operator<<(std::ostream &stream, const Options &opts) {
    return opts.print(stream);
  }
};
