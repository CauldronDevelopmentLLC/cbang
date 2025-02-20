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

#include "CommandLine.h"

#include <cbang/Exception.h>
#include <cbang/json/Writer.h>
#include <cbang/log/Logger.h>

#include <cstdlib>

using namespace std;
using namespace cb;


CommandLine::CommandLine() {
  SmartPointer<Option> opt;

  pushCategory("Informational");

  opt = add("help", 0, this, &CommandLine::usageAction,
            "Print help screen or help on a particular option and exit.");
  opt->setType(Option::TYPE_STRING);
  opt->setOptional();

  add("json-help", 0, this, &CommandLine::jsonHelpAction,
      "Print help in JSON format and exit.");

  add("verbose", 'v', this, &CommandLine::incVerbosityAction,
      "Increase verbosity level.");

  add("quiet", 'q', this, &CommandLine::quietAction,
      "Set verbosity to zero.");

  add("license", 0, this, &CommandLine::licenseAction,
      "License information and exit.");

  popCategory();
}


void CommandLine::add(const string &name, SmartPointer<Option> option) {
  option->setDefaultType(Option::TYPE_BOOLEAN);

  // Short name
  if (option->getShortName())
    Options::add(string("-") + option->getShortName(), option);

  // Long name
  if (!name.empty())
    Options::add(string("--") + option->getName(), option);
}


const SmartPointer<Option> &CommandLine::get(const string &key) const {
  bool shortOpt = 1 < key.length() && key[0] == '-' && key[1] != '-';
  bool longOpt  = 2 < key.length() && key[0] == '-' && key[1] == '-';

  try {
    if (shortOpt || longOpt) return Options::get(key);
    return Options::get("--" + key);

  } catch (const Exception &e) {
    // If its not in the command line options try the keywords.
    if (keywords) {
      if (shortOpt) return keywords->get(key.substr(1));
      if (longOpt)  return keywords->get(key.substr(2));
    }
    throw;
  }
}


int CommandLine::parse(int argc, char *argv[]) {
  return parse(vector<string>(argv, argv + argc));
}


int CommandLine::parse(const vector<string> &args) {
  name = args[0];
  bool configSet = false;
  unsigned i;

  for (i = 1; i < args.size();) {
    string optionStr = args[i];
    SmartPointer<Option> option;

    if (allowExtraOpts && optionStr == "--") {
      i++;
      break;
    }

    if (optionStr.empty() || optionStr[0] != '-') {
      if (allowConfigAsFirstArg && !configSet && !optionStr.empty()) {
        option = get("--config");

        if (option.isSet()) {
          option->set(option->parse(args[i++]));
          configSet = true;
          continue;
        }
      }

      if (allowPositionalArgs) {
        positionalArgs.push_back(args[i++]);
        continue;
      }
    }

    // Split --arg=value
    auto pos = optionStr.find('=');
    if (pos != string::npos) optionStr = optionStr.substr(0, pos);

    try {
      if (allowSingleDashLongOpts && 1 < optionStr.size() &&
          optionStr[1] != '-' && !has(optionStr))
        option = get(string("-") + optionStr);
      else option = get(optionStr);

    } catch (const Exception &e) {
      if (warnOnInvalidArgs) {
        LOG_WARNING("Ignoring invalid or unsupported argument '"
                    << args[i++] << "'");
        continue;
      }

      THROWC("Invalid argument '" << args[i] << "'", e);
    }

    option->parseCommandLine(i, args);
  }

  return i;
}


int CommandLine::usageAction(Option &option) {
  if (option.hasValue()) {
    string name = option.toString();
    SmartPointer<Option> opt;

    if (has(string("--") + name)) opt = get(string("--") + name);
    else if (has(string("-") + name)) opt = get(string("-") + name);
    else if (keywords && keywords->has(name)) opt = keywords->get(name);
    else THROW("Unrecognized command line option or keyword '" << name << "'");

    opt->printHelp(cout) << endl;

  } else usage(cout, name);

  exit(0);
  return -1;
}


int CommandLine::jsonHelpAction() {
  if (keywords) {
    JSON::Writer writer(cout);
    keywords->dump(writer);
  }

  exit(0);
  return -1;
}


void CommandLine::usage(ostream &stream, const string &name) const {
  stream << "Usage: " << name;

  if (usageArgs.empty()) {
    if (allowConfigAsFirstArg) stream << " [[--config] <filename>]";

    stream << " [OPTIONS]...";
    if (allowExtraOpts) stream << " [-- OPTIONS]";

  } else stream << ' ' << usageArgs;

  stream << "\n\nOPTIONS:\n";

  printHelp(stream, true);
  stream << "\n\n";

  if (showKeywordOpts && keywords) {
    stream << "\nConfiguration options:\n";
    String::fill(stream, "The following options can be specified in a "
                 "configuration file or on the command line using the "
                 "following syntax:\n");
    stream <<
      "\n"
      "    --<option> <value>\n"
      "\n"
      "  or:\n"
      "\n"
      "    --<option>=<value>\n"
      "\n"
      "  or when marking boolean values true:\n"
      "\n"
      "    --<option>";

    keywords->printHelp(stream);

    stream << endl;
  }

  // Extra usage lines added by the application
  if (usageExtras.size()) stream << endl;
  for (auto &line: usageExtras) stream << line << endl;
}


int CommandLine::licenseAction() {
  if (licenseText.empty()) cout << "Unspecified" << endl;
  for (auto &text: licenseText) cout << text << endl << endl;
  exit(0);
  return -1;
}


int CommandLine::incVerbosityAction() {
  Logger::instance().setVerbosity(Logger::instance().getVerbosity() + 1);
  return 0;
}


int CommandLine::quietAction() {
  Logger::instance().setVerbosity(0);
  return 0;
}
