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

#include <iostream>

#include <cbang/String.h>
#include <cbang/ServerApplication.h>
#include <cbang/ApplicationMain.h>

#include <cbang/log/Logger.h>
#include <cbang/time/Timer.h>

#include <cbang/script/Server.h>
#include <cbang/script/Environment.h>
#include <cbang/script/Functor.h>

using namespace std;
using namespace cb;
using namespace cb::Script;


void hello(const Context &ctx) {
  ctx.stream << "Hello World!\n";
}


void print(const Context &ctx) {
  Arguments::const_iterator it = ctx.args.begin();
  if (it != ctx.args.end()) it++; // Skip first

  while (it != ctx.args.end())
    ctx.stream << "'" << *it++ << "' ";

  ctx.stream << '\n';
}


class App : public ServerApplication {
  Server server;

public:
  App() : ServerApplication("Hello World Server"), server(getName()) {

    for (int i = 0; i < 10; i++)
      server.addListenPort(String::printf("127.0.0.1:999%d", i));

    server.set("prompt", "$(date '%Y/%m/%d %H:%M:%S')$ ");
    server.set("greeting", "$(clear)Welcome to the Hello World Server\n\n");
    server.add(new Functor("hello", hello, 0, 0, "Print Hello World!"));
    server.add(new Functor("print", ::print, 0, 0, "Print args"));
  }

  void run() {
    server.start();
    while (!quit) Timer::sleep(0.1);
    server.join();
  }
};


int main(int argc, char *argv[]) {
  return cb::doApplication<App>(argc, argv);
}
