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

#include <cbang/ApplicationMain.h>
#include <cbang/Application.h>
#include <cbang/Catch.h>

#include <cbang/openssl/SSLContext.h>
#include <cbang/openssl/KeyGenPacifier.h>
#include <cbang/openssl/Certificate.h>

#include <cbang/event/Base.h>
#include <cbang/event/Event.h>
#include <cbang/event/DNSBase.h>
#include <cbang/event/Client.h>
#include <cbang/event/WebServer.h>

#include <cbang/acmev2/Account.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/log/Logger.h>

using namespace cb;
using namespace std;


void readKey(KeyPair &key, const string &filename) {
  if (SystemUtilities::exists(filename)) {
    LOG_INFO(1, "Reading " << filename);
    key.readPrivate(SystemUtilities::read(filename));

  } else {
    LOG_INFO(1, "Generating " << filename);
    key.generateRSA(4096, 65537, new KeyGenPacifier(cout));
    cout << endl;
    key.writePrivate(*SystemUtilities::oopen(filename, 0600));
  }
}


class App : public Application {
  SmartPointer<SSLContext> sslCtx;

  KeyPair serverKey;
  KeyPair clientKey;

  Event::Base base;
  Event::DNSBase dns;
  Event::Client client;
  Event::WebServer server;

  ACMEv2::Account account;

  string domain = "example.com";

public:
  App() :
    Application("ACMEv2 Tool", App::_hasFeature), sslCtx(new SSLContext),
    base(true), dns(base), client(base, dns, sslCtx), server(options, base),
    account(client) {

    options.pushCategory("ACME v2");
    options.addTarget("domain", domain);
    options.popCategory();

    account.addOptions(options);

    // Modify Option defaults
    options["log-short-level"].setDefault(true);

    // Ignore SIGPIPE
    ::signal(SIGPIPE, SIG_IGN);

    // Handle exit signals
    addSignalEvent(SIGINT);
    addSignalEvent(SIGTERM);
  }


  static bool _hasFeature(int feature) {
    switch (feature) {
    case FEATURE_INFO: return true;
    case FEATURE_PRINT_INFO:
    case FEATURE_SIGNAL_HANDLER:
      return false;
    default: return Application::_hasFeature(feature);
    }
  }


  // From Application
  void run() {
    readKey(serverKey, "server.key");
    readKey(clientKey, "client.key");

    string clientChain;
    if (SystemUtilities::exists("client.chain")) {
      LOG_INFO(1, "Reading client.chain");
      clientChain = SystemUtilities::read("client.chain");
    }

    auto cb =
      [this] (ACMEv2::KeyCert &keyCert) {
        keyCert.getChain().write(*SystemUtilities::oopen("client.chain"));
        base.loopExit();
      };

    account.simpleInit(serverKey, clientKey, domain, clientChain, server, cb);

    if (!account.certsReadyForRenewal()) {
      LOG_INFO(1, "Certificate not due for renewal");
      return;
    }

    server.init();

    base.dispatch();
  }


  void addSignalEvent(int sig) {
    auto event = base.newSignal(sig, this, &App::signalEvent);
    event->setPriority(0);
    event->add();
  }


  void signalEvent(Event::Event &e, int signal, unsigned flags) {
    base.loopExit();
  }
};


int main(int argc, char *argv[]) {return cb::doApplication<App>(argc, argv);}
