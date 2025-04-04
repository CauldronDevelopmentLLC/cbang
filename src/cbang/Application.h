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

#include "SmartPointer.h"

#include <cbang/config/Options.h>
#include <cbang/config/CommandLine.h>

#include <cbang/xml/Reader.h>

#include <cbang/util/Version.h>
#include <cbang/util/Features.h>

#include <cbang/os/ExitSignalHandler.h>

#include <atomic>
#include <string>

namespace cb {
  class Logger;
  class Option;
}


namespace cb {
  namespace XML {class Writer;}

  class Application : public Features, protected ExitSignalHandler {
  public:
    enum {
      FEATURE_PROCESS_CONTROL,
      FEATURE_CONFIG_FILE,
      FEATURE_DEBUGGING,
      FEATURE_INFO,
      FEATURE_PRINT_INFO,
      FEATURE_SIGNAL_HANDLER,
      FEATURE_LAST,
    };

  protected:
    Options options; // Must be first
    XML::Reader configReader;
    Logger &logger;
    CommandLine cmdLine;

    const std::string name;
    Version version;
    std::string runDirectoryRegKey;

    bool configRotate           = true;
    uint32_t configRotateMax    = 16;
    std::string configRotateDir = "configs";

    bool initialized = false;
    bool configured  = false;
    std::atomic<bool> quit;

    uint64_t startTime;

  public:
    Application(const std::string &name,
                hasFeature_t hasFeature = Application::_hasFeature);
    virtual ~Application();

    static bool _hasFeature(int feature);

    Options &getOptions() {return options;}
    const Options &getOptions() const {return options;}
    CommandLine &getCommandLine() {return cmdLine;}
    Logger &getLogger() {return logger;}
    XML::Reader &getXMLReader() {return configReader;}

    const std::string &getName() const {return name;}
    const Version &getVersion() const {return version;}
    void setVersion(const Version &version) {this->version = version;}

    bool isConfigured() const {return configured;}
    virtual bool shouldQuit() const {return quit;}
    virtual void requestExit() {quit = true;}

    uint64_t getStartTime() const {return startTime;}
    uint64_t getUptime() const;

    virtual int init(int argc, char *argv[]);
    virtual void afterCommandLineParse() {}
    virtual void initialize() {}
    virtual void run() {}
    virtual void printInfo() const;
    virtual void write(XML::Writer &writer, uint32_t flags = 0) const;
    virtual std::ostream &print(std::ostream &stream) const;
    virtual void usage(std::ostream &stream, const std::string &name) const;
    virtual void openConfig(const std::string &filename = std::string());
    virtual void saveConfig(const std::string &filename = std::string()) const;

    virtual void writeConfig(std::ostream &stream, uint32_t flags = 0) const;

  protected:
    // Command line actions
    virtual int printAction();
    virtual int infoAction();
    virtual int versionAction();
    virtual int configAction(Option &option);

    // From SignalHandler
    void handleSignal(int sig) override;
  };
}
