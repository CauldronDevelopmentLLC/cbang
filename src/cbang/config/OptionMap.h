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

#include <cbang/SmartPointer.h>
#include <cbang/enum/Enumeration.h>
#include <cbang/xml/HandlerFactory.h>
#include <cbang/log/Logger.h>

#include <string>
#include <iostream>
#include <type_traits>


namespace cb {
  class Option;

  /// A base class for configuration option handling
  class OptionMap : public XML::Handler {
    std::string xmlValue;
    bool xmlValueSet = false;
    bool setDefault  = false;
    bool autoAdd     = false;
    bool allowReset  = false;

  public:
    bool getAutoAdd() const {return autoAdd;}
    void setAutoAdd(bool x) {autoAdd = x;}
    bool getAllowReset() const {return allowReset;}
    void setAllowReset(bool x) {allowReset = x;}

    void add(SmartPointer<Option> option);
    SmartPointer<Option> add(const std::string &name, const std::string &help,
                             const SmartPointer<Constraint> &constraint = 0);
    SmartPointer<Option> add(const std::string &name, const char shortName = 0,
                             SmartPointer<OptionActionBase> action = 0,
                             const std::string &help = "");

    template <typename T>
    SmartPointer<Option> add(const std::string &name, const char shortName,
                             T *obj, typename OptionAction<T>::member_t member,
                             const std::string &help = "") {
      return add(name, shortName, new OptionAction<T>(obj, member), help);
    }

    template <typename T>
    SmartPointer<Option> add(const std::string &name, const char shortName,
                             T *obj,
                             typename BareOptionAction<T>::member_t member,
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
    SmartPointer<Option> addTarget(const std::string &name, T &target,
      const std::string &help = "", char shortName = 0) {
      auto option = add(name, shortName, 0, help);
      bind(name, target);
      setOptionDefault(*option, target);
      return option;
    }

    Option &operator[](const std::string &key) const {return *get(key);}

    void set(Option &option, const JSON::ValuePtr &value);
    void set(const std::string &name, const std::string &value,
      bool setDefault = false);

    // Virtual interface
    virtual void add(const std::string &name, SmartPointer<Option> option) = 0;
    virtual bool remove(const std::string &key) = 0;
    virtual bool has(const std::string &key) const = 0;
    virtual bool local(const std::string &key) const {return true;}
    virtual const SmartPointer<Option> &localize(const std::string &key)
    {return get(key);}
    virtual const SmartPointer<Option> &get(const std::string &key) const = 0;
    virtual void alias(const std::string &name, const std::string &alias) = 0;

    // From XML::Handler
    void startElement(
      const std::string &name, const XML::Attributes &attrs) override;
    void endElement(const std::string &name) override;
    void text(const std::string &text) override;
    void cdata(const std::string &data) override;

  protected:
    SmartPointer<Option> tryLocalize(const std::string &name);
  };
}
