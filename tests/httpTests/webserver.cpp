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

#include <cbang/http/Server.h>
#include <cbang/http/Handler.h>
#include <cbang/http/Context.h>
#include <cbang/http/Connection.h>

#include <cbang/time/Timer.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


class App : public ServerApplication, public Handler {
  Server server;

public:
  App() :
    ServerApplication("Web Server"), cb::HTTP::Handler(".*"), server(options) {

    // Add handler
    server.addHandler(this);
    server.addListenPort(IPAddress("0.0.0.0:8080"));
  }

  // From ServerApplication
  void run() {
    server.start();
    while (!quit) Timer::sleep(0.1);
    server.join();
  }

  // From Handler
  void buildResponse(Context *ctx) {
    Connection &con = ctx->getConnection();
    Request &request = con.getRequest();
    const URI &uri = request.getURI();
    Response &response = con.getResponse();

    response.setContentTypeFromExtension(uri.getPath());

    if (uri.getPath() == "/hello.html")
      con << "<http><body><b>Hello World!</b></body></html>" << flush;
    else if (uri.getPath() != "/empty.html")
      con << "<http><body><b>OK</b></body></html>" << flush;
  }
};


int main(int argc, char *argv[]) {
  return cb::doApplication<App>(argc, argv);
}
