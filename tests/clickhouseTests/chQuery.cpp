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

// Offline driver for the API ClickHouse handlers.  Builds an API whose
// ClickHouse::Client talks to a fake ClickHouse, an HTTP::Server on
// 127.0.0.1 inside this program which records each request it is sent and
// answers with the scenario's canned responses.  The API request is
// dispatched and the response printed, then the ClickHouse requests.  No
// ClickHouse, no external network.
//
//   chQuery <fixture.yaml> <scenario>

#include <cbang/Catch.h>
#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/json/JSON.h>
#include <cbang/json/YAMLReader.h>
#include <cbang/config/CommandLine.h>
#include <cbang/api/API.h>
#include <cbang/db/clickhouse/Client.h>
#include <cbang/http/Client.h>
#include <cbang/http/Server.h>
#include <cbang/http/Request.h>
#include <cbang/http/RequestParams.h>
#include <cbang/http/RequestErrorHandler.h>
#include <cbang/net/Socket.h>
#include <cbang/net/URI.h>
#include <cbang/event/Base.h>
#include <cbang/log/Logger.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <deque>
#include <vector>

using namespace cb;
using namespace std;


namespace {
  // A canned ClickHouse response
  struct Reply {
    unsigned status = 200;
    string code; // X-ClickHouse-Exception-Code
    string body;
    bool hold = false; // Never answer, so the client times out
  };

  deque<Reply> replies;
  vector<string> requests;


  void push(const string &body) {Reply r; r.body = body; replies.push_back(r);}


  void push(unsigned status, const string &code, const string &body) {
    Reply r;
    r.status = status;
    r.code   = code;
    r.body   = body;
    replies.push_back(r);
  }


  void hold() {Reply r; r.hold = true; replies.push_back(r);}


  // A JSONCompact result.  ``cols`` are name, type pairs.
  string result(const vector<pair<string, string>> &cols,
                const string &data) {
    string meta;
    for (auto &col: cols)
      meta += string(meta.empty() ? "" : ", ") + "{\"name\": \"" +
        col.first + "\", \"type\": \"" + col.second + "\"}";

    return "{\"meta\": [" + meta + "], \"data\": " + data + ", \"rows\": " +
      String(JSON::Reader::parse(data)->size()) + ", \"statistics\": "
      "{\"elapsed\": 0.001, \"rows_read\": 1, \"bytes_read\": 1}}\n";
  }


  string error(unsigned code, const string &msg) {
    return SSTR("Code: " << code << ". DB::Exception: " << msg
                << " (version 26.3.37.3 (official build))\n");
  }


  // The fake ClickHouse
  bool clickHouse(HTTP::Request &req) {
    ostringstream str;
    str << "\nCH: " << req.getMethod() << ' ' << req.getURI().getPath();
    for (auto &p: req.getURI())
      str << "\nPARAM " << p.first << ": " << p.second;
    for (auto name: {"X-ClickHouse-User", "X-ClickHouse-Key"})
      if (req.inHas(name)) str << '\n' << name << ": " << req.inFind(name);
    str << "\nBODY: " << req.getInput();
    requests.push_back(str.str());

    if (replies.empty()) THROW("Unexpected ClickHouse request");
    Reply reply = replies.front();
    replies.pop_front();

    if (reply.hold) return true;

    if (!reply.code.empty())
      req.outSet("X-ClickHouse-Exception-Code", reply.code);
    req.outSet("Content-Type", reply.code.empty() ?
               "application/json; charset=UTF-8" : "text/plain; charset=UTF-8");
    req.reply((HTTP::Status::enum_t)reply.status, reply.body);

    return true;
  }


  // Map a scenario name to (method, path), an optional request body, and
  // queue its canned response(s).  Returns false for an unknown scenario.
  bool setup(const string &s, string &method, string &path, string &body,
             string &contentType, string &header, bool &refused,
             bool &noClient, bool &reader) {
    method = "GET";

    if (s == "Ok") {
      path = "/ok";
      push(""); // Not a query, no result

    } else if (s == "OkRows") {
      path = "/ok-select";
      push(result({{"1", "UInt8"}}, "[[1]]"));

    } else if (s == "Pass") {
      path = "/pass";
      push(result({{"id", "UInt64"}}, "[[1], [2]]"));

    } else if (s == "ListScalar") {
      path = "/names";
      push(result({{"name", "String"}}, R"([["Alice"], ["Bob"]])"));

    } else if (s == "ListDict") {
      path = "/users";
      push(result({{"id", "UInt64"}, {"name", "Nullable(String)"}},
                  R"([[1, "A"], [2, null]])"));

    } else if (s == "ListEmpty") {
      path = "/users";
      push(result({{"id", "UInt64"}, {"name", "Nullable(String)"}}, "[]"));

    } else if (s == "HList") {
      path = "/grid";
      push(result({{"a", "UInt8"}, {"b", "Array(String)"}},
                  R"([[1, ["x"]], [3, []]])"));

    } else if (s == "HListEmpty") {
      path = "/grid";
      push(result({{"a", "UInt8"}, {"b", "Array(String)"}}, "[]"));

    } else if (s == "Dict") {
      path = "/user/42";
      push(result({{"id", "UInt32"}, {"name", "String"}}, R"([[42, "Bob"]])"));

    } else if (s == "DictNotFound") {
      path = "/user/99";
      push(result({{"id", "UInt32"}, {"name", "String"}}, "[]"));

    } else if (s == "DictTooManyRows") {
      path = "/user/1";
      push(result({{"id", "UInt32"}, {"name", "String"}},
                  R"([[1, "A"], [1, "B"]])"));

    } else if (s == "One") {
      path = "/count";
      push(result({{"count()", "UInt64"}}, "[[7]]"));

    } else if (s == "OneNotFound") {
      path = "/count";
      push(result({{"count()", "UInt64"}}, "[]"));

    } else if (s == "OneColumns") {
      path = "/count";
      push(result({{"a", "UInt8"}, {"b", "UInt8"}}, "[[1, 2]]"));

    } else if (s == "Bool") {
      path = "/active";
      push(result({{"greater(count(), 0)", "UInt8"}}, "[[1]]"));

    } else if (s == "U64") {
      path = "/max";
      push(result({{"max(id)", "UInt64"}}, "[[18446744073709551615]]"));

    } else if (s == "S64") {
      path = "/min";
      push(result({{"min(delta)", "Int64"}}, "[[-9223372036854775808]]"));

    } else if (s == "Into") {
      path = "/into";
      push(result({{"top", "UInt64"}}, "[[18446744073709551615]]"));
      push(result({{"name", "String"}}, R"([["Zed"]])"));

    } else if (s == "Params") {
      path = "/events?since=2026-09-01T00:00:00Z";
      push(result({{"day", "Date"}, {"type", "String"}, {"events", "UInt64"}},
                  R"([["2026-09-01", "wu", 3]])"));

    } else if (s == "ParamsSet") {
      path = "/events?since=2026-09-01T00:00:00.5%2B02:00&type="
        "it's%09a%5Ctest";
      push(result({{"day", "Date"}, {"type", "String"}, {"events", "UInt64"}},
                  "[]"));

    } else if (s == "Format") {
      method = "POST";
      path   = "/format";
      contentType = "application/json";
      body = R"({"s": "back\\slash tab\t nl\n 'single' \"double\" é☃😀",
        "e": "", "n": null, "t": true, "f": false,
        "i": -9223372036854775808, "u": 18446744073709551615,
        "d": 1.5, "a": ["it's", "back\\slash", "tab\t", "\\N", ""],
        "an": [[1, null], []], "m": {"k'ey": "v\\al", "n": "\n"},
        "c": "a:b"})";
      push(""); // return: ok

    } else if (s == "NoType") path = "/untyped/5";
    else if (s == "Missing") path = "/missing";

    else if (s == "BinaryRef") {
      method = "PUT";
      path   = "/binary";
      body   = "bytes";
      contentType = "application/octet-stream";

    } else if (s == "Escape") {
      path = "/escape";
      push(result({{"json", "String"}, {"ok", "UInt8"}}, "[]"));

    } else if (s == "Settings") {
      path = "/settings?limit=100";
      push(result({{"id", "UInt64"}}, "[[1]]"));

    } else if (s == "SettingsSet") {
      path = "/settings?limit=100&rows=18446744073709551615";
      push(result({{"id", "UInt64"}}, "[[1]]"));

    } else if (s == "Writable") {
      path = "/write";
      push("");

    } else if (s == "Reader") {
      path   = "/ok-select";
      reader = true;
      push("");

    } else if (s == "ReaderWritable") {
      path   = "/write";
      reader = true;
      push("");

    } else if (s == "ReaderInsert") {
      method = "POST";
      path   = "/insert?batch=9";
      contentType = "application/json";
      body   = R"({"type": "wu", "n": 1})";
      reader = true;
      push("");

    } else if (s == "Error") {
      path = "/error";
      push(400, "62", error(62, "Syntax error: failed at position 1 (SELEC): "
                            "SELEC 1. Expected one of: Query. (SYNTAX_ERROR)"));

    } else if (s == "ServerTimeout") {
      path = "/slow";
      push(408, "159", error(159, "Timeout exceeded: elapsed 1001.5 ms, "
                             "maximum: 1000 ms. (TIMEOUT_EXCEEDED)"));

    } else if (s == "Timeout") {
      path = "/slow";
      hold();

    } else if (s == "Refused") {
      path = "/count";
      refused = true;

    } else if (s == "InsertObject") {
      method = "POST";
      path   = "/insert?batch=7";
      contentType = "application/json";
      body = R"({"type": "wu", "n": "3", "machine": "m1"})";
      push("");

    } else if (s == "InsertArray") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = R"([{"type": "wu", "n": 1}, {"n": 2, "type": "o'k\\"}])";
      push("");

    } else if (s == "InsertSet") {
      method = "POST";
      path   = "/insert?batch=8";
      contentType = "application/json";
      body = R"([{"type": "wu", "n": 1, "source": "client", "batch": 99}])";
      push("");

    } else if (s == "InsertRowInvalid") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = R"([{"type": "wu", "n": 1}, {"type": "wu", "n": "x"}])";

    } else if (s == "InsertRowMissing") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = R"([{"type": "wu", "n": 1}, {"n": 2}, {"type": "toolongtype"}])";

    } else if (s == "InsertUnknownField") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = R"([{"type": "wu", "n": 1, "bogus": true}])";

    } else if (s == "InsertNotObject") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = R"([{"type": "wu", "n": 1}, 5])";

    } else if (s == "InsertNotList") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = R"("rows")";

    } else if (s == "InsertMaxRows") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = R"([{"type": "a", "n": 1}, {"type": "b", "n": 2},
                 {"type": "c", "n": 3}, {"type": "d", "n": 4}])";

    } else if (s == "InsertBadJSON") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = R"([{"type": "wu", "n": )";

    } else if (s == "InsertBadJSONRaw") {
      method = "POST";
      path   = "/insert-open";
      contentType = "application/x-ndjson";
      body = "{\"type\": \"wu\"}\n{\"type\": \"wu\"}\n";

    } else if (s == "InsertEmpty") {
      method = "POST";
      path   = "/insert";
      contentType = "application/json";
      body = "[]";

    } else if (s == "InsertNoWait") {
      method = "POST";
      path   = "/insert-open";
      header = "Idempotency-Key: batch-17";
      contentType = "application/json";
      body = R"([{"anything": "goes", "x": [1, 2]}])";
      push("");

    } else if (s == "InsertRejected") {
      method = "POST";
      path   = "/insert-open";
      contentType = "application/json";
      body = R"({"zzz": 1})";
      push(400, "117", error(117, "Unknown field found while parsing "
                             "JSONEachRow format: zzz: (at row 1)\n: While "
                             "executing WaitForAsyncInsert. (INCORRECT_DATA)"));

    } else if (s == "InsertDataError") {
      // Some data errors are a 500, so the code decides
      method = "POST";
      path   = "/insert-open";
      contentType = "application/json";
      body = R"({"b": "maybe"})";
      push(500, "467", error(467, "Invalid boolean value, should be "
                             "true/false, 1/0, but it starts with the '\"' "
                             "character: (while reading the value of key b): "
                             "(at row 1)\n: While executing "
                             "WaitForAsyncInsert. (CANNOT_PARSE_BOOL)"));

    } else if (s == "InsertOwnError") {
      // A 4xx which is not about the data is not the client's
      method = "POST";
      path   = "/insert-open";
      contentType = "application/json";
      body = R"({"a": 1})";
      push(400, "62", error(62, "Syntax error: failed at position 1: "
                            "(SYNTAX_ERROR)"));

    } else if (s == "InsertServerError") {
      method = "POST";
      path   = "/insert-open";
      contentType = "application/json";
      body = R"({"a": 1})";
      push(404, "60", error(60, "Table default.events does not exist. "
                            "(UNKNOWN_TABLE)"));

    } else if (s == "InsertUnavailable") {
      method = "POST";
      path   = "/insert-open";
      contentType = "application/json";
      body = R"({"a": 1})";
      refused = true;

    } else if (s == "InsertTimeout") {
      method = "POST";
      path   = "/insert-open";
      contentType = "application/json";
      body = R"({"a": 1})";
      hold();

    } else if (s == "NoClient") noClient = true;
    else if (s == "Load") {} // The config fails to load
    else return false;

    return true;
  }


  // Bind ``socket``, or ``server`` if given, to a free local port
  SockAddr bindFree(Socket &socket, HTTP::Server *server = 0) {
    for (unsigned port = 18123; port < 18623; port++) {
      SockAddr addr((uint32_t)0x7f000001, (uint16_t)port);

      try {
        if (server) server->addListenPort(addr);
        else socket.open(Socket::REUSEADDR, addr);
        return addr;
      } catch (const Exception &e) {socket.close();}
    }

    THROW("No free port");
  }
}


int main(int argc, char *argv[]) {
  try {
    // Keep logs off stdout and deterministic.  Quiet the fake ClickHouse's
    // error replies.  --verbosity may override.
    Logger::instance().setScreenStream(cerr);
    Logger::instance().setVerbosity(0);
    Logger::instance().setLogTime(false);
    Logger::instance().setLogColor(false);
    Exception::printLocations    = false;
    Exception::enableStackTraces = false;

    Options options;
    options.add("db", "ClickHouse database")->set("grow");
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

    string method, path, body, contentType, header;
    bool refused = false, noClient = false, reader = false;
    if (!setup(scenario, method, path, body, contentType, header, refused,
               noClient, reader))
      THROW("Unknown scenario: " << scenario);

    // Real sockets, so the pool thread must be able to wake the loop
    Event::Base base;

    // The fake ClickHouse, and a bound but not listening port which refuses
    // connections
    Socket closed;
    HTTP::Server server(base);
    server.addHandler(new HTTP::RequestFunctionHandler(clickHouse));
    auto addr = refused ? bindFree(closed) : bindFree(closed, &server);

    SmartPointer<ClickHouse::Client> ch =
      new ClickHouse::Client(new HTTP::Client(base));
    ch->setURL(URI("http", "127.0.0.1", addr.getPort()).toString());
    ch->setUser("tester");
    ch->setPassword("secret");
    ch->setDatabase("testdb");
    ch->setTimeout(1);

    if (reader) {
      ch->setReaderUser("reader");
      ch->setReaderPassword("rsecret");
    }

    API::API api(options);
    if (!noClient) api.setClickHouse(ch);
    api.load(JSON::YAMLReader::parseFile(configPath));

    HTTP::RequestParams params;
    params.method = HTTP::Method::parse(method, HTTP::Method::HTTP_GET);
    params.uri    = URI(path);

    params.hdrs = new HTTP::Headers;
    if (!contentType.empty()) params.hdrs->insert("Content-Type", contentType);
    if (!header.empty()) {
      auto colon = header.find(':');
      params.hdrs->insert(header.substr(0, colon),
                          String::trim(header.substr(colon + 1)));
    }

    HTTP::Request req(params);
    if (!body.empty()) req.getInputBuffer().add(body.data(), body.length());

    // Dispatch as the server does, so handler throws reply with a status
    HTTP::RequestErrorHandler errorHandler(api);
    errorHandler(req);

    while (!req.isReplying()) base.loopOnce();

    cout << (unsigned)req.getResponseCode() << "\n";

    ostringstream hs;
    req.getOutputHeaders().write(hs);
    string headers = hs.str();
    headers.erase(remove(headers.begin(), headers.end(), '\r'), headers.end());
    cout << headers;

    cout << req.getOutput();

    for (auto &request: requests) cout << '\n' << request;
    cout << endl;
    if (!replies.empty()) THROW(replies.size() << " replies not requested");

    return 0;
  } CATCH_ERROR;

  return 1;
}
