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
#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/json/YAMLReader.h>
#include <cbang/config/CommandLine.h>
#include <cbang/api/API.h>
#include <cbang/db/maria/Connector.h>
#include <cbang/http/Request.h>
#include <cbang/http/RequestParams.h>
#include <cbang/http/RequestErrorHandler.h>
#include <cbang/http/Client.h>
#include <cbang/oauth2/Providers.h>
#include <cbang/net/URI.h>
#include <cbang/event/Base.h>
#include <cbang/event/SubprocessPool.h>
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


  // Map a scenario name to (method, path), an optional request body, and
  // queue its canned result(s).  Returns false for an unknown scenario.
  bool setup(const string &s, string &method, string &path, string &body,
             string &contentType, string &auth) {
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

    } else if (s == "Blob") {
      method = "PUT";
      path   = "/blob";
      body   = string("\x00\x01\x02\r\n\xff\xfe bytes\x00", 14);
      contentType = "application/octet-stream";
      FakeDB::push(Response()); // OK, no result set

    } else if (s == "Upload") {
      method = "POST";
      path   = "/photo";
      contentType = "multipart/form-data; boundary=XbOuNdX";
      body =
        "--XbOuNdX\r\n"
        "Content-Disposition: form-data; name=\"photo\"; "
        "filename=\"cat.png\"\r\n"
        "Content-Type: image/png\r\n\r\n" +
        string("PNG\0DATA", 8) +
        "\r\n--XbOuNdX--\r\n";
      FakeDB::push(Response()); // OK, no result set

    } else if (s == "Pipe") {
      path = "/pipe/7";
      push(Response()); // LogAccess: OK, no result set
      push(result({{"id", FakeDB::LONG}, {"name", FakeDB::STRING}},
                  {{Cell("7"), Cell("Bob")}})); // UserGet

    } else if (s == "BlobPipe") {
      path = "/blob-pipe";
      push(result({{"data", FakeDB::BLOB}, {"type", FakeDB::STRING}},
                  {{Cell(string("IMG\0DATA", 8)), Cell("image/png")}}));
      push(Response()); // StoreCopy: OK, no result set

    } else if (s == "Resize") {
      method = "PUT";
      path   = "/resize/3/64";
      body   = string("ORIG\0BYTES", 10);
      contentType = "application/octet-stream";
      push(Response()); // ImageSet: OK
      push(result({{"data", FakeDB::BLOB}},
                  {{Cell(string("ORIG\0BYTES", 10))}})); // ImageGet
      push(Response()); // ImageSetSize: OK

    } else if (s == "AssetUpload") {
      method = "POST";
      path   = "/asset?id=42";
      contentType = "multipart/form-data; boundary=XbOuNdX";
      body =
        "--XbOuNdX\r\n"
        "Content-Disposition: form-data; name=\"file\"; "
        "filename=\"pic.png\"\r\n"
        "Content-Type: image/png\r\n\r\n" +
        string("PNG\0DATA", 8) +
        "\r\n--XbOuNdX--\r\n";
      push(result({{"id", FakeDB::STRING}}, {{Cell("42")}})); // AssetSave

    } else if (s == "Binary") {
      path = "/image";
      push(result({{"data", FakeDB::BLOB}},
                  {{Cell(string("IMG\0\xff\r\nBYTES", 12))}}));

    } else if (s == "BigBinary") {
      // A blob far larger than any single fetch buffer: it must come back whole,
      // not truncated/empty (the bug that returned 0-byte assets over ~64 KiB).
      path = "/image";
      string big;
      for (unsigned i = 0; i < 100000; i++) big.push_back((char)(i & 0xff));
      push(result({{"data", FakeDB::BLOB}}, {{Cell(big)}}));

    } else if (s == "BinaryTypeCol") {
      path = "/image";
      push(result({{"data", FakeDB::BLOB}, {"type", FakeDB::STRING}},
                  {{Cell(string("PNG\0DATA", 8)), Cell("image/png")}}));

    } else if (s == "BinaryCTKey") {
      path = "/image-gif"; // the content-type key beats the second column
      push(result({{"data", FakeDB::BLOB}, {"type", FakeDB::STRING}},
                  {{Cell(string("GIF\0DATA", 8)), Cell("image/png")}}));

    } else if (s == "BinaryNotFound") {
      path = "/image";
      push(result({{"data", FakeDB::BLOB}}, {}));

    } else if (s == "OptNull") {
      method = "POST";
      path   = "/opt";
      push(Response()); // OptSave: OK, no result set

    } else if (s == "OptSet") {
      method = "POST";
      path   = "/opt?a=hi";
      push(Response()); // OptSave: OK, no result set

    } else if (s == "StrictMissing") {
      path = "/strict";
      // No response: the missing {args.nope} errors before any query

    } else if (s == "BlobEmbed") {
      method = "PUT";
      path   = "/blob-embed";
      body   = "bytes";
      contentType = "application/octet-stream";
      // No response: the bind/placeholder count mismatch errors first

    } else if (s == "Float") {
      // FLOAT column: regressed to 0 because the binary->text conversion was
      // width-limited by a zero-length bound buffer.  Must round-trip in full.
      path = "/scalar";
      push(result({{"val", FakeDB::FLOAT}}, {{Cell("1268930")}}));

    } else if (s == "FloatNegative") {
      path = "/scalar";
      push(result({{"val", FakeDB::FLOAT}}, {{Cell("-0.5")}}));

    } else if (s == "Double") {
      path = "/scalar";
      push(result({{"val", FakeDB::DOUBLE}}, {{Cell("123456789.5")}}));

    } else if (s == "FloatNull") {
      path = "/scalar";
      push(result({{"val", FakeDB::FLOAT}}, {{Cell()}})); // SQL NULL

    } else if (s == "Decimal") {
      // NEWDECIMAL is text on the wire, so it never regressed -- a control
      // proving the fix is specific to the binary float types.
      path = "/scalar";
      push(result({{"val", FakeDB::NEWDECIMAL}}, {{Cell("0.0856")}}));

    } else if (s == "BigString") {
      // Longer than MIN_RESULT_BUFFER: forces the grow + fetch_column + rebind
      // path, so a stale rebind (reading freed memory) would corrupt this.
      path = "/scalar";
      push(result({{"val", FakeDB::STRING}}, {{Cell(string(600, 'A'))}}));

    } else if (s == "WU") {
      // The real ProjectWUList shape: the FLOAT credit sits between an integer
      // and a decimal, both of which always worked.
      path = "/wu";
      push(result({{"user", FakeDB::STRING}, {"team", FakeDB::LONG},
                   {"credit", FakeDB::FLOAT}, {"days", FakeDB::NEWDECIMAL},
                   {"code", FakeDB::STRING}},
                  {{Cell("Fahnatic"), Cell("224497"), Cell("1268930"),
                    Cell("0.0856"), Cell("Ok")}}));

    } else if (s == "SessionArgError") {
      // A root async session SQL lookup runs, then a nested endpoint's arg
      // validation throws 400.  The throw must reply cleanly, not escape the
      // session callback into the DB event loop.  Authorization supplies a
      // session id so the session handler takes its async DB-lookup branch.
      path = "/item/abc"; // 'abc' fails the u32 arg
      auth = "testsid";
      push(Response()); // AuthSession: OK, no result set

    } else return false;

    return true;
  }
}


int main(int argc, char *argv[]) {
  try {
    // Keep logs off stdout and deterministic; command line options may
    // override, e.g. --verbosity=5 (debug logging requires a DEBUG build)
    Logger::instance().setScreenStream(cerr);
    Logger::instance().setLogTime(false);
    Logger::instance().setLogColor(false);
    Exception::printLocations    = false;
    Exception::enableStackTraces = false;

    Options options;
    options.add("scripts", "Path to exec scripts.");
    Logger::instance().addOptions(options);

    CommandLine cmdLine;
    cmdLine.setKeywordOptions(&options);
    cmdLine.setUsageArgs("<fixture.yaml> <scenario>");
    cmdLine.parse(argc, argv);

    auto &args = cmdLine.getPositionalArgs();
    if (args.size() != 2) {
      cmdLine.usage(cerr, argv[0]);
      return 1;
    }

    string configPath = args[0];
    string scenario   = args[1];

    auto slash = configPath.find_last_of('/');
    string dir = slash == string::npos ? "." : configPath.substr(0, slash);
    if (!options["scripts"].hasValue()) options["scripts"].set(dir + "/scripts");

    Event::Base base(false);
    API::API api(options);

    string method, path, body, contentType, auth;
    FakeDB::reset();
    if (!setup(scenario, method, path, body, contentType, auth))
      THROW("Unknown scenario: " << scenario);

    api.setDBConnector(new MariaDB::Connector(base));
    api.setProcPool(new Event::SubprocessPool(base));

    // The 'session' handler registers only with these three set
    if (!auth.empty()) {
      api.setClient(new HTTP::Client(base));
      api.setOAuth2Providers(new OAuth2::Providers);
      api.setSessionManager(new HTTP::SessionManager);
    }

    api.load(JSON::YAMLReader::parseFile(configPath));

    HTTP::RequestParams params;
    params.method = HTTP::Method::parse(method, HTTP::Method::HTTP_GET);
    params.uri    = URI(path);

    params.hdrs = new HTTP::Headers;
    if (!contentType.empty()) params.hdrs->insert("Content-Type", contentType);
    if (!auth.empty())        params.hdrs->insert("Authorization", auth);

    HTTP::Request req(params);
    if (!body.empty()) req.getInputBuffer().add(body.data(), body.length());

    // Dispatch as the server does, so handler throws reply with a status
    HTTP::RequestErrorHandler errorHandler(api);
    errorHandler(req);

    for (unsigned i = 0; !req.isReplying() && i < 100000; i++) base.loopOnce();
    if (!req.isReplying()) THROW("Request never replied");

    cout << (unsigned)req.getResponseCode() << "\n";

    ostringstream hs;
    req.getOutputHeaders().write(hs);
    string headers = hs.str();
    headers.erase(remove(headers.begin(), headers.end(), '\r'), headers.end());
    cout << headers;

    cout << req.getOutput();

    auto &queries = FakeDB::queries();
    auto &binds   = FakeDB::binds();
    for (unsigned i = 0; i < queries.size(); i++) {
      cout << "\nSQL: " << queries[i];
      for (unsigned j = 0; j < binds[i].size(); j++) {
        auto &bind = binds[i][j];
        cout << "\nBIND[" << j << "]: "
             << (bind == "\\N" ? "NULL" : MariaDB::DB::toHex(bind));
      }
    }

    return 0;
  } CATCH_ERROR;

  return 1;
}
