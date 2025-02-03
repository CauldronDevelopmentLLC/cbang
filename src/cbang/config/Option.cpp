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

// Proxy constructor
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


const string &Option::getDefault() const {
  if (flags & DEFAULT_SET_FLAG) return defaultValue;
  if (!parent.isNull() && parent->hasValue()) return parent->toString();
  return defaultValue;
}


void Option::setDefault(const string &defaultValue) {
  setDefault(defaultValue, TYPE_STRING);
}


void Option::setDefault(const char *defaultValue) {
  setDefault(string(defaultValue), TYPE_STRING);
}


void Option::setDefault(int64_t defaultValue) {
  setDefault(String(defaultValue), TYPE_INTEGER);
}


void Option::setDefault(double defaultValue) {
  setDefault(String(defaultValue), TYPE_DOUBLE);
}


void Option::setDefault(bool defaultValue) {
  setDefault(String(defaultValue), TYPE_BOOLEAN);
}


void Option::setDefault(const JSON::Value &value) {
  switch (value.getType()) {
  case JSON::ValueType::JSON_LIST: {
    string defaultValue;

    for (auto &v: value) {
      if (!defaultValue.empty()) defaultValue += " ";
      defaultValue += v->asString();
    }

    setDefault(defaultValue);
    type = TYPE_STRINGS;
    break;
  }

  case JSON::ValueType::JSON_BOOLEAN: setDefault(value.getBoolean()); break;
  case JSON::ValueType::JSON_NUMBER:  setDefault(value.getNumber());  break;
  default:                            setDefault(value.asString());   break;
  }
}


void Option::clearDefault() {
  defaultValue.clear();
  flags &= ~DEFAULT_SET_FLAG;
}


bool Option::hasDefault() const {
  return flags & DEFAULT_SET_FLAG || (!parent.isNull() && parent->hasValue());
}


bool Option::isDefault() const {
  return hasDefault() && isSet() && value == getDefault();
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

  if (config.has("default")) {
    auto type = this->type;
    setDefault(*config.get("default"));
    // Don't change previously set type
    if (type != TYPE_STRING) this->type = type;
  }

  if (config.has("type")) type = OptionType::parse(config.getString("type"));

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
  if (!isSet() && value.empty()) return;  // Don't run action

  flags &= ~SET_FLAG;
  value.clear();

  if (hasAction()) (*action)(*this);
}


void Option::unset() {
  flags &= ~DEFAULT_SET_FLAG;
  defaultValue.clear();
  reset();
}


void Option::set(const string &value) {
  if (isDeprecated()) {
    LOG_WARNING("Option '" << name << "' has been deprecated: " << help);
    return;
  }
  if (isSet() && this->value == value) return;

  uint32_t oldFlags = flags;
  string oldValue = this->value;

  flags |= SET_FLAG;
  this->value = value;

  // Clear the command line flag
  flags &= ~COMMAND_LINE_FLAG;

  try {
    validate();
  } catch (const Exception &e) {
    flags = oldFlags;
    this->value = oldValue;

    string errStr = string("Invalid value for option '") + name + "'";

    if (Options::warnWhenInvalid)
      LOG_WARNING(errStr << ": " << e.getMessage());

    else {
      ostringstream str;
      str << errStr << ".  Option help:\n";
      printHelp(str);
      THROWC(str.str(), e);
    }
  }

  if (hasAction()) (*action)(*this);
}


void Option::set(int64_t value) {set(String(value));}
void Option::set(double value) {set(String(value));}
void Option::set(bool value) {set(String(value));}


void Option::set(const strings_t &values) {
  string value;

  for (unsigned i = 1; i < values.size(); i++) {
    if (!value.empty()) value += " ";
    value += values[i];
  }

  set(value);
}


void Option::set(const integers_t &values) {
  string value;

  for (unsigned i = 1; i < values.size(); i++) {
    if (!value.empty()) value += " ";
    value += String(values[i]);
  }

  set(value);
}


void Option::set(const doubles_t &values) {
  string value;

  for (unsigned i = 1; i < values.size(); i++) {
    if (!value.empty()) value += " ";
    value += String(values[i]);
  }

  set(value);
}


void Option::append(const string &value) {
  if (isSet() && !this->value.empty()) set(this->value + " " + value);
  else set(value);
}


void Option::append(int64_t value) {append(String(value));}
void Option::append(double value) {append(String(value));}
bool Option::hasValue() const {return isSet() || hasDefault();}
bool Option::toBoolean() const {return parseBoolean(toString());}


const string &Option::toString() const {
  if (isSet()) return value;
  else if (hasDefault()) return getDefault();
  else if (getType() == TYPE_STRINGS) return value;
  THROW("Option '" << name << "' has no default and is not set.");
}


int64_t Option::toInteger() const {return parseInteger(toString());}
double Option::toDouble() const {return parseDouble(toString());}


Option::strings_t Option::toStrings() const {return parseStrings(toString());}


Option::integers_t Option::toIntegers() const {
  return parseIntegers(toString());
}


Option::doubles_t Option::toDoubles() const {return parseDoubles(toString());}


bool Option::toBoolean(bool defaultValue) const {
  return hasValue() ? toBoolean() : defaultValue;
}


const string &Option::toString(const string &defaultValue) const {
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


bool Option::parseBoolean(const string &value) {
  return String::parseBool(value);
}


int64_t Option::parseInteger(const string &value) {
  return String::parseS64(value);
}


double Option::parseDouble(const string &value) {
  return String::parseDouble(value);
}


Option::strings_t Option::parseStrings(const string &value) {
  strings_t result;
  String::tokenize(value, result, " \t\r\n");
  return result;
}


Option::integers_t Option::parseIntegers(const string &value) {
  integers_t result;
  strings_t tokens;

  String::tokenize(value, tokens);

  for (auto &token: tokens)
    result.push_back(String::parseS32(token));

  return result;
}


Option::doubles_t Option::parseDoubles(const string &value) {
  doubles_t result;
  strings_t tokens;

  String::tokenize(value, tokens);

  for (auto &token: tokens)
    result.push_back(String::parseDouble(token));

  return result;
}


void Option::validate() const {
  switch (type) {
  case TYPE_BOOLEAN:  checkConstraint(toBoolean());  break;
  case TYPE_STRING:   checkConstraint(value);        break;
  case TYPE_INTEGER:  checkConstraint(toInteger());  break;
  case TYPE_DOUBLE:   checkConstraint(toDouble());   break;
  case TYPE_STRINGS:  checkConstraint(toStrings());  break;
  case TYPE_INTEGERS: checkConstraint(toIntegers()); break;
  case TYPE_DOUBLES:  checkConstraint(toDoubles());  break;
  default: THROW("Invalid type " << type);
  }
}


void Option::parse(unsigned &i, const vector<string> &args) {
  string arg = args[i++];
  string name;
  string value;
  bool hasValue = false;

  string::size_type pos = arg.find('=');
  if (pos != string::npos) {
    name = arg.substr(0, pos);
    value = arg.substr(pos + 1);
    hasValue = true;

  } else name = arg;

  // Required
  if (hasValue) set(value);

  else if (type == TYPE_BOOLEAN) set(true);

  else if (!isOptional()) {
    if (i == args.size()) {
      ostringstream str;
      str << "Missing required argument for option:\n";
      printHelp(str, true);
      LOG_WARNING(str.str());

    } else set(args[i++]);

  } else if (i < args.size() && args[i][0] != '-') set(args[i++]);

  else if (hasAction()) (*action)(*this); // No arg
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
    if (hasDefault()) stream << '=' << getDefault();
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


void Option::writeBoolean(JSON::Sink &sink, const string &value) {
  sink.writeBoolean(parseBoolean(value));
}


void Option::writeInteger(JSON::Sink &sink, const string &value) {
  int64_t x = parseInteger(value);

  if (JSON_MIN_INT < x && x < JSON_MAX_INT) sink.write(x);
  else sink.write(SSTR("0x" << hex << x));
}


void Option::writeDouble(JSON::Sink &sink, const string &value) {
  sink.write(parseDouble(value));
}


void Option::writeStrings(JSON::Sink &sink, const string &value) {
  strings_t l = parseStrings(value);

  sink.beginList();
  for (auto &s: l) sink.append(s);
  sink.endList();
}


void Option::writeIntegers(JSON::Sink &sink, const string &value) {
  integers_t l = parseIntegers(value);

  sink.beginList();
  for (auto i: l) {
    sink.beginAppend();
    if (JSON_MIN_INT < i && i < JSON_MAX_INT) sink.write(i);
    else sink.write(SSTR("0x" << hex << i));
  }
  sink.endList();
}


void Option::writeDoubles(JSON::Sink &sink, const string &value) {
  doubles_t l = parseDoubles(value);

  sink.beginList();
  for (auto d: l) {
    sink.beginAppend();
    sink.append(d);
  }
  sink.endList();
}


void Option::writeValue(JSON::Sink &sink, const string &value) const {
  switch (type) {
  case TYPE_BOOLEAN:  writeBoolean(sink,  value); break;
  case TYPE_STRING:   sink.write(         value); break;
  case TYPE_INTEGER:  writeInteger(sink,  value); break;
  case TYPE_DOUBLE:   writeDouble(sink,   value); break;
  case TYPE_STRINGS:  writeStrings(sink,  value); break;
  case TYPE_INTEGERS: writeIntegers(sink, value); break;
  case TYPE_DOUBLES:  writeDoubles(sink,  value); break;
  default: THROW("Invalid type " << type);
  }
}


void Option::write(JSON::Sink &sink, bool config) const {
  if (config) return writeValue(sink, toString());

  sink.beginDict();

  if (!getHelp().empty()) sink.insert("help", getHelp());

  if (hasValue()) {
    sink.beginInsert("value");
    writeValue(sink, toString());
  }

  if (hasDefault()) {
    sink.beginInsert("default");
    writeValue(sink, getDefault());
  }

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

  if (shortName)      sink.insert("short",   string(1, shortName));
  if (!help.empty())  sink.insert("help",    help);
  if (isOptional())   sink.insertBoolean("optional",   true);
  if (isObscured())   sink.insertBoolean("obscured",   true);
  if (isDeprecated()) sink.insertBoolean("deprecated", true);

  if (hasDefault()) {
    sink.beginInsert("default");

    switch (type) {
    case TYPE_BOOLEAN: sink.writeBoolean(toBoolean()); break;
    case TYPE_INTEGER: sink.writeBoolean(toInteger()); break;
    case TYPE_DOUBLE:  sink.writeBoolean(toDouble());  break;
    default:
      if (isPlural()) {
        sink.beginList();
        auto strings = toStrings();

        for (auto &s: strings) {
          switch (type) {
          case TYPE_STRINGS:  sink.append(s);               break;
          case TYPE_INTEGERS: sink.append(parseInteger(s)); break;
          case TYPE_DOUBLES:  sink.append(parseDouble(s));  break;
          default: THROW("Unsupported plural option type: " << type);
          }
        }

        sink.endList();

      } else sink.write(defaultValue);
      break;
    }
  }

  if (aliases.size()) {
    sink.insertList("aliases");
    for (auto &alias: aliases)
      sink.append(alias);
    sink.endList();
  }

  if (constraint.isSet()) constraint->dump(sink);

  sink.endDict();
}


void Option::setDefault(const string &value, OptionType type) {
  defaultValue = value;
  flags |= DEFAULT_SET_FLAG;
  this->type = type;

  if (defaultSetAction.isSet()) (*defaultSetAction)(*this);
}
