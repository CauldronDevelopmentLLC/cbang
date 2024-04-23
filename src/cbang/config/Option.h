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

#pragma once

#include "OptionType.h"
#include "OptionAction.h"
#include "Constraint.h"

#include <cbang/SmartPointer.h>
#include <cbang/xml/Handler.h>

#include <ostream>
#include <string>
#include <vector>
#include <set>
#include <cstdint>


namespace cb {
  namespace JSON {class Sink; class Value;}

  /**
   * A Configuration option.  Holds option defaults, user values, and parses
   * option strings.  Can also call an OptionAction when an option is set.
   */
  class Option : public OptionType::Enum {
  public:
    typedef std::vector<std::string> strings_t;
    typedef std::vector<int64_t>     integers_t;
    typedef std::vector<double>      doubles_t;

    typedef enum {
      DEFAULT_SET_FLAG  = 1 << 0,
      SET_FLAG          = 1 << 1,
      OPTIONAL_FLAG     = 1 << 2, // Only used for command line options
      OBSCURED_FLAG     = 1 << 3,
      COMMAND_LINE_FLAG = 1 << 4,
      DEPRECATED_FLAG   = 1 << 5,
    } flags_t;

  protected:
    std::string name;
    char        shortName = 0;
    OptionType  type = TYPE_STRING;
    std::string defaultValue;
    std::string help;
    std::string value;
    uint32_t    flags = 0;

    typedef std::set<std::string> aliases_t;
    aliases_t aliases;

    SmartPointer<Option>           parent;
    SmartPointer<OptionActionBase> action;
    SmartPointer<OptionActionBase> defaultSetAction;
    SmartPointer<Constraint>       constraint;

  public:
    Option(const SmartPointer<Option> &parent);
    Option(const std::string &name, char shortName = 0,
           SmartPointer<OptionActionBase> action = 0,
           const std::string &help = "");
    Option(const std::string &name, const std::string &help,
           const SmartPointer<Constraint> &constraint = 0);
    Option(const std::string &name, const JSON::Value &config);

    const std::string &getName() const {return name;}
    char getShortName() const {return shortName;}

    void setType(OptionType type) {this->type = type;}
    OptionType getType() const {return type;}
    const std::string getTypeString() const;

    const std::string &getDefault() const;
    void setDefault(const std::string &defaultValue);
    void setDefault(const char *defaultValue);
    void setDefault(int64_t  defaultValue);
    void setDefault(uint64_t defaultValue) {setDefault((int64_t)defaultValue);}
    void setDefault(int32_t  defaultValue) {setDefault((int64_t)defaultValue);}
    void setDefault(uint32_t defaultValue) {setDefault((int64_t)defaultValue);}
    void setDefault(double   defaultValue);
    void setDefault(bool     defaultValue);
    void setDefault(const JSON::Value &value);
    void clearDefault();
    bool hasDefault() const;
    bool isDefault() const;

    void setOptional()    {flags |= OPTIONAL_FLAG;}
    void setObscured()    {flags |= OBSCURED_FLAG;}
    void setCommandLine() {flags |= COMMAND_LINE_FLAG;}
    void setDeprecated()  {flags |= DEPRECATED_FLAG; clearDefault();}

    bool isOptional()    const {return flags & OPTIONAL_FLAG;}
    bool isObscured()    const {return flags & OBSCURED_FLAG;}
    bool isPlural()      const {return type >= TYPE_STRINGS;}
    bool isCommandLine() const {return flags & COMMAND_LINE_FLAG;}
    bool isDeprecated()  const {return flags & DEPRECATED_FLAG;}
    bool isHidden()      const;

    const std::string &getHelp() const {return help;}
    void setHelp(const std::string &help) {this->help = help;}

    /// This function should only be called by Options::alias()
    void addAlias(const std::string &alias) {aliases.insert(alias);}
    const aliases_t &getAliases() const {return aliases;}

    void setAction(const SmartPointer<OptionActionBase> &action);
    void setDefaultSetAction(const SmartPointer<OptionActionBase> &action);
    void setConstraint(const SmartPointer<Constraint> &constraint);

    void configure(const JSON::Value &value);
    void reset();
    void unset();
    void set(const std::string &value);
    void set(const char *value) {set(std::string(value));}
    void set(int64_t  value);
    void set(uint64_t value) {set((int64_t)value);}
    void set(int32_t  value) {set((int64_t)value);}
    void set(uint32_t value) {set((int64_t)value);}
    void set(double   value);
    void set(bool     value);
    void set(const strings_t &values);
    void set(const integers_t &values);
    void set(const doubles_t &values);

    void append(const std::string &value);
    void append(int64_t value);
    void append(double value);

    bool isSet() const {return flags & SET_FLAG;}
    bool hasValue() const;

    bool       toBoolean()  const;
    const std::string &toString() const;
    int64_t    toInteger()  const;
    double     toDouble()   const;
    strings_t  toStrings()  const;
    integers_t toIntegers() const;
    doubles_t  toDoubles()  const;

    bool       toBoolean(bool defaultValue) const;
    const std::string &toString(const std::string &defaultValue) const;
    int64_t    toInteger(int64_t defaultValue) const;
    double     toDouble(double defaultValue) const;
    strings_t  toStrings(const strings_t &defaultValue) const;
    integers_t toIntegers(const integers_t &defaultValue) const;
    doubles_t  toDoubles(const doubles_t &defaultValue) const;

    static bool       parseBoolean (const std::string &value);
    static int64_t    parseInteger (const std::string &value);
    static double     parseDouble  (const std::string &value);
    static strings_t  parseStrings (const std::string &value);
    static integers_t parseIntegers(const std::string &value);
    static doubles_t  parseDoubles (const std::string &value);

    template <typename T>
    void checkConstraint(T value) const {
      if (constraint.isSet()) constraint->validate(value);
      if (parent.isSet()) parent->checkConstraint(value);
    }

    void validate() const;

    bool hasAction() const {return action.isSet();}

    void parse(unsigned &i, const std::vector<std::string> &args);
    std::ostream &printHelp(std::ostream &stream, bool cmdLine = true) const;
    std::ostream &print(std::ostream &stream) const;

    operator const std::string &() const {return toString();}

    static void writeBoolean (JSON::Sink &sink, const std::string &value);
    static void writeInteger (JSON::Sink &sink, const std::string &value);
    static void writeDouble  (JSON::Sink &sink, const std::string &value);
    static void writeStrings (JSON::Sink &sink, const std::string &value);
    static void writeIntegers(JSON::Sink &sink, const std::string &value);
    static void writeDoubles (JSON::Sink &sink, const std::string &value);

    void writeValue(JSON::Sink &sink, const std::string &value) const;
    void write(JSON::Sink &sink, bool config = false) const;
    void write(XML::Handler &handler, uint32_t flags) const;
    void dump(JSON::Sink &sink) const;

  protected:
    void setDefault(const std::string &value, OptionType type);
  };

  inline std::ostream &operator<<(std::ostream &stream, const Option &o) {
    return stream << o.toString();
  }

  inline std::string operator+(const std::string &s, const Option &o) {
    return s + o.toString();
  }
};
