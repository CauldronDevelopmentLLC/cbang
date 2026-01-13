/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include "SignalManager.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <csignal>

using namespace std;
using namespace cb;


namespace {
  void sig_handler(int sig) {SignalManager::instance().signal(sig);}
}


void SignalManager::addHandler(int sig, SignalHandler *handler) {
  if (handler && handlers.find(sig) != handlers.end())
    THROW("Signal " << sig << " already has handler.");

  ::signal(sig, sig_handler);
  if (handler) handlers[sig] = handler;
}


void SignalManager::removeHandler(int sig) {
  handlers.erase(sig);
  ::signal(sig, SIG_DFL);
}


const char *SignalManager::signalString(int sig) {
  switch (sig) {
  case SIGABRT: return "SIGABRT";
  case SIGFPE:  return "SIGFPE";
  case SIGILL:  return "SIGILL";
  case SIGINT:  return "SIGINT";
  case SIGSEGV: return "SIGSEGV";
  case SIGTERM: return "SIGTERM";
  case SIGHUP:  return "SIGHUP";
  case SIGQUIT: return "SIGQUIT";
#ifndef _WIN32
  case SIGKILL: return "SIGKILL";
  case SIGPIPE: return "SIGPIPE";
  case SIGALRM: return "SIGALRM";
  case SIGUSR1: return "SIGUSR1";
  case SIGUSR2: return "SIGUSR2";
  case SIGCHLD: return "SIGCHLD";
  case SIGCONT: return "SIGCONT";
  case SIGSTOP: return "SIGSTOP";
  case SIGBUS:  return "SIGBUS";
#endif
  default:      return "UNKNOWN";
  }
}


void SignalManager::signal(int sig) {
  LOG_INFO(2, "Caught signal " << signalString(sig) << '(' << sig << ')');

  auto it = handlers.find(sig);
  if (it != handlers.end()) it->second->handleSignal(sig);
}
