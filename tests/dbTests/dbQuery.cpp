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

// Offline driver for the API query path.  Builds an API with a real
// MariaDB::Connector/EventDB whose libmariadb calls are satisfied by the
// linked-in fake (FakeMariaDB.*).  A scenario selects the endpoint and the
// canned result set(s); the request is dispatched and the captured response
// plus the SQL actually sent are printed.  No database, no network.
//
//   dbQuery <fixture.yaml> <scenario>

#include "FakeMariaDB.h"

#include <cbang/Catch.h>
#include <cbang/String.h>
#include <cbang/json/YAMLReader.h>
#include <cbang/config/Options.h>
#include <cbang/api/API.h>
#include <cbang/db/maria/Connector.h>
#include <cbang/http/Request.h>
#include <cbang/http/RequestParams.h>
#include <cbang/net/URI.h>
#include <cbang/event/Base.h>
#include <cbang/log/Logger.h>

#include <mysql/mysqld_error.h>

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace cb;
using namespace std;

using FakeDB::Cell;
using FakeDB::Col;
using FakeDB::Result;
using FakeDB::Response;
using Rows = vector<vector<Cell>>;


namespace {
  Result result(vector<Col> cols, Rows rows) {
    Result r;
    r.cols = cols;
    r.rows = rows;
    return r;
  }

  void push(const Result &r) {Response resp; resp.results = {r}; FakeDB::push(resp);}


  // Map a scenario name to (method, path) and queue its canned result(s).
  // Returns false for an unknown scenario.
  bool setup(const string &s, string &method, string &path) {
    method = "GET";

    if (s == "Dict") {
      path = "/dict/42";
      push(result({{"id", FakeDB::LONG}, {"name", FakeDB::STRING}},
                  {{Cell("42"), Cell("Bob")}}));

    } else if (s == "DictNotFound") {
      path = "/dict/99";
      push(result({{"id", FakeDB::LONG}, {"name", FakeDB::STRING}}, {}));

    } else if (s == "DictNull") {
      path = "/dict/42";
      push(result({{"id", FakeDB::LONG}, {"name", FakeDB::STRING}},
                  {{Cell("42"), Cell()}})); // name is SQL NULL

    } else if (s == "ListScalar") {
      path = "/names";
      push(result({{"name", FakeDB::STRING}}, {{Cell("Alice")}, {Cell("Bob")}}));

    } else if (s == "ListDict") {
      path = "/list";
      push(result({{"id", FakeDB::LONG}, {"name", FakeDB::STRING}},
                  {{Cell("1"), Cell("A")}, {Cell("2"), Cell("B")}}));

    } else if (s == "One") {
      path = "/one";
      push(result({{"n", FakeDB::LONG}}, {{Cell("7")}}));

    } else if (s == "U64") {
      path = "/u64";
      push(result({{"id", FakeDB::LONGLONG}}, {{Cell("123")}}));

    } else if (s == "Bool") {
      path = "/flag";
      push(result({{"active", FakeDB::TINY}}, {{Cell("1")}}));

    } else if (s == "Fields") {
      path = "/full/5";
      Response resp;
      resp.results = {
        result({{"id", FakeDB::LONG}, {"name", FakeDB::STRING}},
               {{Cell("5"), Cell("Z")}}),
        result({{"team", FakeDB::STRING}}, {{Cell("x")}, {Cell("y")}}),
      };
      FakeDB::push(resp);

    } else if (s == "HList") {
      path = "/grid";
      push(result({{"a", FakeDB::STRING}, {"b", FakeDB::STRING}},
                  {{Cell("1"), Cell("2")}, {Cell("3"), Cell("4")}}));

    } else if (s == "Error") {
      path = "/err";
      Response resp;
      resp.errnoVal = ER_SIGNAL_EXCEPTION; // -> HTTP 400
      resp.error    = "boom";
      resp.sqlstate = "45000";
      FakeDB::push(resp);

    } else return false;

    return true;
  }
}


int main(int argc, char *argv[]) {
  try {
    if (argc < 3) {
      cerr << "Usage: " << argv[0] << " <fixture.yaml> <scenario>" << endl;
      return 1;
    }

    string configPath = argv[1];
    string scenario   = argv[2];

    // Keep logs off stdout and deterministic
    Logger::instance().setScreenStream(cerr);
    Logger::instance().setLogTime(false);

    Event::Base base(false);
    Options options;
    API::API api(options);

    string method, path;
    FakeDB::reset();
    if (!setup(scenario, method, path)) THROW("Unknown scenario: " << scenario);

    api.setDBConnector(new MariaDB::Connector(base));
    api.load(JSON::YAMLReader::parseFile(configPath));

    HTTP::RequestParams params;
    params.method = HTTP::Method::parse(method, HTTP::Method::HTTP_GET);
    params.uri    = URI(path);
    HTTP::Request req(params);

    api(req);

    for (unsigned i = 0; !req.isReplying() && i < 100000; i++) base.loopOnce();
    if (!req.isReplying()) THROW("Request never replied");

    cout << (unsigned)req.getResponseCode() << "\n";

    ostringstream hs;
    req.getOutputHeaders().write(hs);
    string headers = hs.str();
    headers.erase(remove(headers.begin(), headers.end(), '\r'), headers.end());
    cout << headers;

    cout << req.getOutput();

    for (auto &q: FakeDB::queries()) cout << "\nSQL: " << q;

    return 0;
  } CATCH_ERROR;

  return 1;
}
