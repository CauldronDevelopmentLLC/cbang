/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "Application.h"

namespace cb {
  class ServerApplication : public Application {
    bool restartChild;
    uint64_t lifeline;

  public:
    enum {
      FEATURE_SERVER = Application::FEATURE_LAST,
      FEATURE_LIFELINE,
      FEATURE_CHECK_OPEN_FILE_LIMIT,
      FEATURE_LAST,
    };

    ServerApplication(const std::string &name,
                      hasFeature_t hasFeature = ServerApplication::_hasFeature);

    static bool _hasFeature(int feature);

    virtual void beforeDroppingPrivileges() {}

    // From Application
    void afterCommandLineParse() override;
    int init(int argc, char *argv[]) override;
    void run() override {}
    bool shouldQuit() const override;

    uint64_t getLifeline() const {return lifeline;}
    bool lostLifeline() const;

  protected:
    // Command line actions
    int chdirAction(Option &option);
    int respawnAction();
    int daemonAction();

    // From SignalHandler
    void handleSignal(int sig) override;
  };
}
