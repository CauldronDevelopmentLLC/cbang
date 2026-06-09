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

// Offline driver for the API handler chain.  Builds an API from a config
// file, dispatches one synthetic request, and prints the captured response —
// no network.  An event loop is pumped so async statements (exec, query)
// complete; synchronous handlers reply in-line.
//
//   apiDispatch <config> <METHOD> <path> [Name: value ...]
//
// The http-root, favicon and scripts options default to "<config-dir>/...".
// Output is the status code, the response headers, then the body.

#include <cbang/Catch.h>
#include <cbang/String.h>
#include <cbang/json/YAMLReader.h>
#include <cbang/config/Options.h>
#include <cbang/api/API.h>
#include <cbang/http/Request.h>
#include <cbang/http/RequestParams.h>
#include <cbang/http/Headers.h>
#include <cbang/event/Base.h>
#include <cbang/event/SubprocessPool.h>
#include <cbang/log/Logger.h>

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace cb;
using namespace std;


int main(int argc, char *argv[]) {
  try {
    if (argc < 4) {
      cerr << "Usage: " << argv[0]
           << " <config> <METHOD> <path> [Name: value ...]" << endl;
      return 1;
    }

    // Keep logs off stdout (which carries the response) and deterministic
    Logger::instance().setScreenStream(cerr);
    Logger::instance().setLogTime(false);

    string configPath = argv[1];
    string method     = argv[2];
    string path       = argv[3];

    auto slash = configPath.find_last_of('/');
    string base = slash == string::npos ? "." : configPath.substr(0, slash);

    Options options;
    options.add("http-root", "")->set(base + "/www");
    options.add("favicon",   "")->set(base + "/www/favicon.ico");
    options.add("scripts",   "")->set(base + "/scripts");

    Event::Base eventBase(false);

    API::API api(options);
    api.setProcPool(new Event::SubprocessPool(eventBase));
    api.load(JSON::YAMLReader::parseFile(configPath));

    HTTP::RequestParams params;
    params.method = HTTP::Method::parse(method, HTTP::Method::HTTP_GET);
    params.uri    = URI(path);

    // Remaining args are request headers in "Name: value" form.
    params.hdrs = new HTTP::Headers;
    for (int i = 4; i < argc; i++) {
      string h = argv[i];
      auto colon = h.find(':');
      if (colon == string::npos) THROW("Expected 'Name: value': " << h);
      params.hdrs->insert(
        String::trim(h.substr(0, colon)), String::trim(h.substr(colon + 1)));
    }

    HTTP::Request req(params);

    api(req);

    // Drive the event loop until an async chain (exec, query) replies
    while (!req.isReplying()) eventBase.loopOnce();

    cout << (unsigned)req.getResponseCode() << "\n";

    ostringstream hs;
    req.getOutputHeaders().write(hs);
    string headers = hs.str();
    headers.erase(remove(headers.begin(), headers.end(), '\r'), headers.end());
    cout << headers;

    cout << req.getOutput();

    return 0;
  } CATCH_ERROR;

  return 1;
}
