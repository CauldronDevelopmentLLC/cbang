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

#include "Option.h"
#include "Options.h"
#include "MinMaxConstraint.h"

#include <cbang/String.h>

#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/Catch.h>
#include <cbang/json/Sink.h>
#include <cbang/json/Value.h>
#include <cbang/json/Integer.h>

using namespace std;
using namespace cb;


Option::Option(const SmartPointer<Option> &parent) :
  name(parent->name), shortName(parent->shortName), type(parent->type),
  help(parent->help), flags(parent->flags & ~(SET_FLAG | DEFAULT_SET_FLAG)),
  aliases(parent->aliases), parent(parent), action(parent->action),
  defaultSetAction(parent->defaultSetAction) {
}


Option::Option(const string &name, const char shortName,
               SmartPointer<OptionActionBase> action, const string &help) :
  name(name), shortName(shortName), help(help), action(action) {}


Option::Option(const string &name, const string &help,
               const SmartPointer<Constraint> &constraint) :
  name(name), help(help), constraint(constraint) {}


Option::Option(const string &name, const JSON::Value &config) :
  name(name) {configure(config);}


void Option::setDefaultType(OptionType type) {this->type = type;}


void Option::setType(OptionType type) {
  flags |= TYPE_SET_FLAG;
  this->type = type;
}


void Option::setTypeFromValue(const JSON::Value &value) {
  switch (value.getType()) {
  case JSON::ValueType::JSON_LIST: {
    if (value.empty()) setType(TYPE_STRINGS);
    else {
      auto &child = value.get(0);

      if (child->isNumber())
        setType(child->isInteger() ? TYPE_INTEGERS : TYPE_DOUBLES);
      else if (child->isString()) setType(TYPE_STRINGS);
      else THROW("Invalid default list member type: " << child->getType());
    }
    break;
  }

  case JSON::ValueType::JSON_STRING:  setType(TYPE_STRING); break;
  case JSON::ValueType::JSON_BOOLEAN: setType(TYPE_BOOLEAN); break;

  case JSON::ValueType::JSON_NUMBER:
    setType(value.isInteger() ? TYPE_INTEGER : TYPE_DOUBLE);
    break;

  default: THROW("Invalid type for option: " << value.getType());
  }
}


const string Option::getTypeString() const {
  switch (type) {
  case TYPE_BOOLEAN:  return "boolean";
  case TYPE_STRING:   return "string";
  case TYPE_INTEGER:  return "integer";
  case TYPE_DOUBLE:   return "double";
  case TYPE_STRINGS:  return "string ...";
  case TYPE_INTEGERS: return "integer ...";
  case TYPE_DOUBLES:  return "double ...";
  default: THROW("Invalid type " << type);
  }
}


const JSON::ValuePtr &Option::getDefault() const {
  if (flags & DEFAULT_SET_FLAG) return defaultValue;
  if (parent.isSet() && parent->hasValue()) return parent->getValue();
  return defaultValue;
}


void Option::setDefault(const JSON::ValuePtr &value) {
  if (value->isNull()) {
    flags &= ~DEFAULT_SET_FLAG;
    defaultValue = 0;
    return;
  }

  if (isTypeSet()) validate(*value);

  defaultValue = value;
  flags |= DEFAULT_SET_FLAG;

  // Set type from default if not already set
  if (!isTypeSet()) setTypeFromValue(*value);

  if (defaultSetAction.isSet()) (*defaultSetAction)(*this);
}


void Option::setDefault(const string &value) {
  if (isPlural()) setDefault(parse(value));
  else setDefault(JSON::Factory().create(value));
}


void Option::setDefault(int64_t value) {
  setDefault(JSON::Factory().create(value));
}


void Option::setDefault(double value) {
  setDefault(JSON::Factory().create(value));
}


void Option::setDefault(bool value) {
  setDefault(JSON::Factory().createBoolean(value));
}


void Option::clearDefault() {
  defaultValue = 0;
  flags &= ~DEFAULT_SET_FLAG;
}


bool Option::hasDefault() const {
  return flags & DEFAULT_SET_FLAG || (parent.isSet() && parent->hasValue());
}


bool Option::isDefault() const {
  return hasDefault() && isSet() && *value == *getDefault();
}


bool Option::isHidden() const {
  return isDeprecated() || name.empty() || name[0] == '_';
}


void Option::setAction(const SmartPointer<OptionActionBase> &action) {
  if (this->action.isSet()) THROW("Option " << name << " action already set");
  this->action = action;
}

void Option::setDefaultSetAction(const SmartPointer<OptionActionBase> &action) {
  if (defaultSetAction.isSet())
    THROW("Option " << name << " default set action already set");
  defaultSetAction = action;
}


void Option::setConstraint(const SmartPointer<Constraint> &constraint) {
  if (this->constraint.isSet())
    THROW("Option " << name << " constraint already set");
  this->constraint = constraint;
}


void Option::configure(const JSON::Value &config) {
  help = config.getString("help", "");

  if (config.hasString("short"))
    shortName = config.getString("short").c_str()[0];

  if (config.has("default")) setDefault(config.get("default"));
  if (config.has("type")) setType(OptionType::parse(config.getString("type")));

  if (config.getBoolean("optional",   false)) setOptional();
  if (config.getBoolean("obscured",   false)) setObscured();
  if (config.getBoolean("deprecated", false)) setDeprecated();

  if (config.hasList("aliases")) {
    auto &aliases = config.getList("aliases");
    for (auto &alias: aliases)
      addAlias(alias->asString());
  }

  bool hasMin = config.hasNumber("min");
  bool hasMax = config.hasNumber("max");

  if (hasMin || hasMax) {
    if (type == TYPE_INTEGER) {
      auto min = config.getS64("min", 0);
      auto max = config.getS64("max", 0);

      if      (!hasMin) setConstraint(new MaxConstraint<int64_t>(max));
      else if (!hasMax) setConstraint(new MinConstraint<int64_t>(min));
      else              setConstraint(new MinMaxConstraint<int64_t>(min, max));

    } else {
      auto min = config.getNumber("min", 0);
      auto max = config.getNumber("max", 0);

      if      (!hasMin) setConstraint(new MaxConstraint<double>(max));
      else if (!hasMax) setConstraint(new MinConstraint<double>(min));
      else              setConstraint(new MinMaxConstraint<double>(min, max));
    }
  }
}


void Option::reset() {
  if (!isSet()) return;  // Don't run action

  flags &= ~SET_FLAG;
  value = 0;

  if (hasAction()) (*action)(*this);
}


void Option::unset() {
  flags &= ~DEFAULT_SET_FLAG;
  defaultValue = 0;
  reset();
}


void Option::set(const JSON::ValuePtr &value) {
  if (isDeprecated()) {
    LOG_WARNING("Option '" << name << "' has been deprecated: " << help);
    return;
  }

  try {
    validate(*value);
  } catch (const Exception &e) {
    string errStr = "Invalid value for option '" + name + "'";

    if (Options::warnWhenInvalid) {
      LOG_WARNING(errStr << ": " << e.getMessage());
      return;

    } else {
      ostringstream str;
      str << errStr << ".  Option help:\n";
      printHelp(str);
      THROWC(str.str(), e);
    }
  }

  if (isSet()) {
    if (isPlural()) {
      if (!value->isList()) value->append(value);
      else for (auto &v: *value) value->append(v);

    } else if (*this->value == *value) return;
  }

  flags |= SET_FLAG;
  flags &= ~COMMAND_LINE_FLAG; // Clear the command line flag

  if (isPlural() && !value->isList()) {
    this->value = JSON::Factory().createList();
    this->value->append(value);

  } else this->value = value;

  if (hasAction()) (*action)(*this);
}


void Option::set(const string &value) {set(JSON::Factory().create(value));}
void Option::set(int64_t       value) {set(JSON::Factory().create(value));}
void Option::set(double        value) {set(JSON::Factory().create(value));}
void Option::set(bool value) {set(JSON::Factory().createBoolean(value));}


JSON::ValuePtr Option::get() const {
  if (isSet())      return value;
  if (hasDefault()) return getDefault();
  if (isPlural())   return JSON::Factory().createList();
  THROW("Option '" << name << "' has no default and is not set.");
}


bool    Option::toBoolean() const {return get()->toBoolean();}
string  Option::toString()  const {return get()->asString();}
int64_t Option::toInteger() const {return get()->getS64();}
double  Option::toDouble()  const {return get()->getNumber();}


Option::strings_t Option::toStrings() const {
  if (!isPlural() && !isString())
    THROW("Option is not a plural type or string");

  strings_t values;

  if (isString()) String::tokenize(toString(), values);
  else for (auto &v: *get()) values.push_back(v->asString());

  return values;
}


Option::integers_t Option::toIntegers() const {
  if (type != TYPE_INTEGERS) THROW("Option is not integers type");
  integers_t values;
  for (auto &v: *get()) values.push_back(v->getS64());
  return values;
}


Option::doubles_t Option::toDoubles() const {
  if (type != TYPE_DOUBLES) THROW("Option is not doubles type");
  doubles_t values;
  for (auto &v: *get()) values.push_back(v->getNumber());
  return values;
}


bool Option::toBoolean(bool defaultValue) const {
  return hasValue() ? toBoolean() : defaultValue;
}


string Option::toString(const string &defaultValue) const {
  return hasValue() ? toString() : defaultValue;
}


int64_t Option::toInteger(int64_t defaultValue) const {
  return hasValue() ? toInteger() : defaultValue;
}


double Option::toDouble(double defaultValue) const {
  return hasValue() ? toDouble() : defaultValue;
}


Option::strings_t Option::toStrings(const strings_t &defaultValue) const {
  return hasValue() ? toStrings() : defaultValue;
}


Option::integers_t Option::toIntegers(const integers_t &defaultValue) const {
  return hasValue() ? toIntegers() : defaultValue;
}


Option::doubles_t Option::toDoubles(const doubles_t &defaultValue) const {
  return hasValue() ? toDoubles() : defaultValue;
}


void Option::validate(const JSON::Value &value) const {
  try {
    if (value.isList()) {
      if (!isPlural()) THROW("Option is not a plural type");
      for (auto &v: value) validate(*v);
      return;
    }

    switch (type) {
    case TYPE_BOOLEAN:
      if (!value.isBoolean()) THROW("Option must be boolean");
      break;

    case TYPE_STRING: case TYPE_STRINGS:
      if (!value.isString()) THROW("Option must be string");
      break;

    case TYPE_INTEGER: case TYPE_INTEGERS:
      if (!value.isInteger()) THROW("Option must be integer");
      break;

    case TYPE_DOUBLE: case TYPE_DOUBLES:
      if (!value.isNumber()) THROW("Option must be number");
      break;

    default: THROW("Invalid option type: " << type);
    }

    if (constraint.isSet()) constraint->validate(value);
    if (parent.isSet()) parent->validate(value);

  } catch (const Exception &e) {
    THROWC("Option '" << name << "' validation failed", e);
  }
}


JSON::ValuePtr Option::parse(const string &value, OptionType type) {
  switch (type) {
  case TYPE_BOOLEAN:
    return JSON::Factory().createBoolean(String::parseBool(value, true));

  case TYPE_STRING: return JSON::Factory().create(value);

  case TYPE_INTEGER:
    return JSON::Factory().create(String::parseS64(value, true));

  case TYPE_DOUBLE:
    return JSON::Factory().create(String::parseDouble(value, true));

  case TYPE_STRINGS: case TYPE_INTEGERS: case TYPE_DOUBLES: {
    strings_t tokens;
    String::tokenize(value, tokens);
    auto list = JSON::Factory().createList();

    for (auto &token: tokens)
      switch (type) {
      case TYPE_STRINGS:  list->append(token);                            break;
      case TYPE_INTEGERS: list->append(String::parseS64   (token, true)); break;
      case TYPE_DOUBLES:  list->append(String::parseDouble(token, true)); break;
      default: break; // Not possible
      }

    return list;
  }

  default: THROW("Invalid option type: " << type);
  }
}


void Option::parseCommandLine(unsigned &i, const vector<string> &args) {
  string arg = args[i++];
  string name;
  string value;
  bool hasValue = false;

  string::size_type pos = arg.find('=');
  if (pos != string::npos) {
    name  = arg.substr(0, pos);
    value = arg.substr(pos + 1);
    hasValue = true;

  } else name = arg;

  // Required
  if (hasValue) set(parse(value));

  else if (type == TYPE_BOOLEAN) set(JSON::Factory().createBoolean(true));

  else if (!isOptional()) {
    if (i == args.size()) {
      ostringstream str;
      str << "Missing required argument for option:\n";
      printHelp(str, true);
      LOG_WARNING(str.str());

    } else set(parse(args[i++], type));

  } else if (i < args.size() && args[i][0] != '-')
    set(parse(args[i++], type));

  else if (hasAction()) (*action)(*this); // No arg

  flags |= COMMAND_LINE_FLAG;
}


ostream &Option::printHelp(ostream &stream, bool cmdLine) const {
  stream << "  ";

  // Short option name
  if (shortName && cmdLine) stream << '-' << shortName;

  // Long option name
  if (name != "") {
    if (shortName && cmdLine)  stream << "|";
    if (cmdLine) stream << "--";
    stream << name;
  }

  // Arg
  if (type != TYPE_BOOLEAN || !cmdLine) {
    stream << ' ' << (isOptional() ? '[' : '<');
    stream << getTypeString();
    if (hasDefault()) stream << '=' << getDefault()->asString();
    stream << (isOptional() ? ']' : '>');
  }

  // Deprecated
  if (isDeprecated()) stream << " (Deprecated)";

  // Help
  unsigned width = 80;
  try {
    const char *ohw = SystemUtilities::getenv("OPTIONS_HELP_WIDTH");
    if (ohw) width = String::parseU32(ohw);
  } CATCH_WARNING;

  stream << '\n';
  String::fill(stream, help, 0, cmdLine ? 6 : 4, width);

  if (constraint.isSet()) {
    stream << '\n';
    String::fill(stream, constraint->getHelp(), 0, cmdLine ? 6 : 4, width);
  }

  return stream;
}


ostream &Option::print(ostream &stream) const {
  stream << String::escapeC(name) << ':';
  if (hasValue()) stream << ' ' << String::escapeC(toString());
  return stream;
}


void Option::write(JSON::Sink &sink, bool config) const {
  if (config) return sink.write(*get());

  sink.beginDict();

  if (!getHelp().empty()) sink.insert("help",    getHelp());
  if (hasValue())         sink.insert("value",   *getValue());
  if (hasDefault())       sink.insert("default", *getDefault());

  sink.insert("type", getTypeString());
  if (shortName)          sink.insert("short", string(1, shortName));
  if (isOptional())       sink.insertBoolean("optional",     true);
  if (isObscured())       sink.insertBoolean("obscured",     true);
  if (isSet())            sink.insertBoolean("set",          true);
  if (isCommandLine())    sink.insertBoolean("command_line", true);
  if (isDeprecated())     sink.insertBoolean("deprecated",   true);
  if (constraint.isSet()) sink.insert("constraint", constraint->getHelp());

  sink.endDict();
}


void Option::write(XML::Handler &handler, uint32_t flags) const {
  XML::Attributes attrs;

  string value = toString();
  if (isObscured() && !(flags & OBSCURED_FLAG)) value = string(5, '*');
  if (!isPlural()) attrs["v"] = value;

  handler.startElement(name, attrs);

  if (isPlural()) handler.text(value);

  handler.endElement(name);
}


void Option::dump(JSON::Sink &sink) const {
  sink.beginDict();

  if (type != TYPE_STRING)
    sink.insert("type", String::toLower(type.toString()));

  if (shortName)      sink.insert("short", string(1, shortName));
  if (!help.empty())  sink.insert("help",  help);
  if (isOptional())   sink.insertBoolean("optional",   true);
  if (isObscured())   sink.insertBoolean("obscured",   true);
  if (isDeprecated()) sink.insertBoolean("deprecated", true);
  if (hasDefault())   sink.insert("default", *defaultValue);

  if (aliases.size()) {
    sink.insertList("aliases");
    for (auto &alias: aliases) sink.append(alias);
    sink.endList();
  }

  if (constraint.isSet()) constraint->dump(sink);

  sink.endDict();
}
