/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "Application.h"

#include <cbang/Info.h>
#include <cbang/Catch.h>

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SystemInfo.h>
#include <cbang/os/SignalManager.h>

#include <cbang/time/Timer.h>
#include <cbang/time/Time.h>

#include <cbang/util/ResourceManager.h>
#include <cbang/log/Logger.h>
#include <cbang/config/Option.h>
#include <cbang/json/Writer.h>
#include <cbang/xml/Writer.h>

#include <sstream>
#include <set>

#ifndef _WIN32
#include <sys/resource.h>
#endif


using namespace std;
using namespace cb;


namespace cb {
  extern void loadResources();
  namespace BuildInfo {
    void addBuildInfo(const char *category);
  }
}


Application::Application(const string &name, hasFeature_t hasFeature) :
  Features(hasFeature), logger(Logger::instance()), name(name),
  quit(false), startTime(Timer::now()) {

  loadResources();

  // Core dumps
#if defined(DEBUG) && !defined(_WIN32)
  // Enable full core dumps in debug mode
  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  setrlimit(RLIMIT_CORE, &rlim);
#endif

#ifndef _WIN32
  // Ignore SIGPIPE by default
  if (hasFeature(FEATURE_SIGNAL_HANDLER))
    SignalManager::instance().ignoreSignal(SIGPIPE);
#endif

#ifndef DEBUG
  // Hide debugging options in non-debug builds
  options.getCategory("Debugging")->setHidden(true);
#endif

  // Options
  if (hasFeature(FEATURE_DEBUGGING)) {
    options.pushCategory("Debugging");
    options.addTarget("stack-traces", Exception::enableStackTraces,
                      "Enable or disable stack traces on errors.");
    options.addTarget("exception-locations", Exception::printLocations,
                      "Enable or disable exception location printing.");
    options.popCategory();
  }

  if (hasFeature(FEATURE_PROCESS_CONTROL)) {
    options.pushCategory("Process Control");
    options.add("service", "Ignore user logout or hangup and interrupt signals"
                )->setDefault(false);
    options.add("priority", "Set the process priority. Valid values are: idle, "
                "low, normal, high or realtime.");
    options.alias("priority", "nice");
    options.popCategory();
  }

  logger.addOptions(options);

  // Command line
  if (hasFeature(FEATURE_CONFIG_FILE)) {
    cmdLine.pushCategory("Configuration");

    cmdLine.add("config", 0, this, &Application::configAction,
                "Set configuration file.")->setDefault("config.xml");

    cmdLine.add("print", 0, this, &Application::printAction,
                "Print configuration and exit.");

    cmdLine.popCategory();
  }

  cmdLine.pushCategory("Informational");

  cmdLine.add("version", 0, this, &Application::versionAction,
              "Print application version and exit.");

  if (hasFeature(FEATURE_INFO))
    cmdLine.add("info", 0, this, &Application::infoAction,
                "Print application and system information and exit.");

  cmdLine.popCategory();
  cmdLine.setKeywordOptions(&options);

  // Info
  if (hasFeature(FEATURE_INFO)) {
    Info &info = Info::instance();
    BuildInfo::addBuildInfo("CBang");

    // Add to info
    SystemInfo::instance().add(info);
    info.add("System", "UTC Offset", String(Time::offset() / 3600));
    info.add("System", "PID",        String(SystemUtilities::getPID()));
    info.add("System", "CWD",        SystemUtilities::getcwd());
    info.add("System", "Exec",       SystemUtilities::getExecutablePath());
  }

  // Load licenses
  auto &licenses = ResourceManager::instance().get("cb/licenses");
  for (unsigned i = 0; licenses.getChild(i); i++)
    cmdLine.addLicenseText(licenses.getChild(i)->getData());
}


Application::~Application() {
#ifdef DEBUG_LEAKS
  // Deallocate singletons
  SingletonDealloc::instance().deallocate();
#endif // DEBUG_LEAKS
}


bool Application::_hasFeature(int feature) {
  switch (feature) {
  case FEATURE_CONFIG_FILE:
  case FEATURE_DEBUGGING:
  case FEATURE_PRINT_INFO:
  case FEATURE_SIGNAL_HANDLER:
    return true;

  default: return false;
  }
}


double Application::getUptime() const {return Timer::now() - startTime;}


int Application::init(int argc, char *argv[]) {
  if (initialized) THROW("Already initialized");
  initialized = true;
  quit = false;

  if (hasFeature(FEATURE_INFO) && !version.toU32()) {
    if (Info::instance().has(name, "Version"))
      version = Version(Info::instance().get(name, "Version"));
    else if (Info::instance().has("Build", "Version"))
      version = Version(Info::instance().get("Build", "Version"));
  }

  // Add args to info, obscuring any obscured options
  if (hasFeature(FEATURE_INFO)) {
    Info &info = Info::instance();
    ostringstream args;
    bool obscureNext = false;
    for (int i = 1; i < argc; i++) {
      if (i) args << " ";

      string arg = argv[i];
      if (2 < arg.length() && arg.substr(0, 2) == "--") {
        string name = arg.substr(2);
        size_t equal = name.find('=');
        if (equal != string::npos) name = name.substr(0, equal);

        if (options.has(name) && options[name].isObscured()) {
          if (equal == string::npos) obscureNext = true;
          else arg = arg.substr(0, equal + 3) +
                 string(arg.length() - equal - 3, '*');
        }

      } else if (obscureNext) {
        arg = string(arg.length(), '*');
        obscureNext = false;
      }

      args << arg;
    }
    info.add(name, "Args", args.str());
  }

  // Parse args
  int ret = cmdLine.parse(argc, argv);
  if (ret == -1) return -1;
  afterCommandLineParse();

  // Load default config
  if (hasFeature(FEATURE_CONFIG_FILE) && cmdLine["--config"].hasValue() &&
      !configured && SystemUtilities::exists(cmdLine["--config"]))
    configAction(cmdLine["--config"]);

  logger.setOptions(options);

  if (hasFeature(FEATURE_PROCESS_CONTROL))
    try {
      if (options["priority"].hasValue())
        SystemUtilities::setPriority(
          ProcessPriority::parse(options["priority"]));
    } CBANG_CATCH_WARNING;

  if (hasFeature(FEATURE_SIGNAL_HANDLER))
    catchExitSignals(); // Also enables SignalManager

  if (hasFeature(FEATURE_PRINT_INFO)) printInfo();

  initialize();
  return ret;
}


void Application::printInfo() const {
  // Print Info
  if (hasFeature(FEATURE_INFO))
    Info::instance().print(
      *LOG_INFO_STREAM(1), 80 - Logger::instance().getHeaderWidth());

  // Write config to log
  if (hasFeature(FEATURE_CONFIG_FILE))
    writeConfig(*LOG_INFO_STREAM(2), 3 < Logger::instance().getVerbosity() ?
                Option::DEFAULT_SET_FLAG : 0);
}


void Application::write(XML::Writer &writer, uint32_t flags) const {
  getOptions().write(writer, flags);
}


ostream &Application::print(ostream &stream) const {
  return stream << options;
}


void Application::usage(ostream &stream, const string &name) const {
  cmdLine.usage(stream, name);
}


void Application::openConfig(const string &_filename) {
  string filename;
  if (_filename.empty()) filename = cmdLine["--config"];
  else filename = _filename;

  configReader.read(filename, &options);
  configured = true;

  // Add config to info
  Info::instance().add(name, "Config", SystemUtilities::absolute(filename));
}


void Application::saveConfig(const string &_filename) const {
  string filename;
  if (_filename.empty()) filename = cmdLine["--config"];
  else filename = _filename;

  if (configRotate)
    SystemUtilities::rotate(filename, configRotateDir, configRotateMax);

  LOG_INFO(1, "Saving configuration to " << filename);
  writeConfig(*LOG_INFO_STREAM(2));
  writeConfig(*SystemUtilities::open(filename, ios::out),
              Option::OBSCURED_FLAG);
}


void Application::writeConfig(ostream &stream, uint32_t flags) const {
  XML::Writer writer(stream, true);
  writer.startElement("config");
  write(writer, flags);
  writer.endElement("config");
}


int Application::printAction() {
  print(*LOG_RAW_STREAM());
  exit(0);
  return -1;
}


int Application::infoAction() {
  Info::instance().print(*LOG_RAW_STREAM());
  exit(0);
  return -1;
}


int Application::versionAction() {
  LOG_RAW(version);
  exit(0);
  return -1;
}


int Application::configAction(Option &option) {
  openConfig(option.toString());
  return 0;
}


void Application::handleSignal(int sig) {
  if (hasFeature(FEATURE_PROCESS_CONTROL) &&
      options["service"].toBoolean() && sig == SIGHUP) {
    LOG_INFO(1, "Service ignoring hangup/logoff signal");
    return;
  }

  requestExit();

  ExitSignalHandler::handleSignal(sig);
}
