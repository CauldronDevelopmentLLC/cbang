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

#include "Client.h"

#include <cbang/config/Options.h>
#include <cbang/http/Client.h>
#include <cbang/event/Base.h>
#include <cbang/event/Event.h>
#include <cbang/net/URI.h>

using namespace std;
using namespace cb;
using namespace cb::ClickHouse;


namespace {
  // Backslash, tab and newline must be escaped in ClickHouse's text formats.
  // In a quoted string, as in an array or map, so must the single quote.
  void escape(string &s, const string &value, bool quoted) {
    for (char c: value)
      switch (c) {
      case '\\': s += "\\\\";                 break;
      case '\t': s += "\\t";                  break;
      case '\n': s += "\\n";                  break;
      case '\'': s += quoted ? "\\'" : "'"; break;
      default:   s.push_back(c);              break;
      }
  }


  // Values inside an array or map literal are quoted and null is NULL
  void format(string &s, const JSON::Value &value, bool quoted) {
    if (value.isNull()) s += quoted ? "NULL" : "\\N";
    else if (value.isBoolean()) s += value.getBoolean() ? "true" : "false";

    else if (value.isString()) {
      if (quoted) s += '\'';
      escape(s, value.getString(), quoted);
      if (quoted) s += '\'';

    } else if (value.isList()) {
      s += '[';
      for (unsigned i = 0; i < value.size(); i++) {
        if (i) s += ',';
        format(s, *value.get(i), true);
      }
      s += ']';

    } else if (value.isDict()) {
      s += '{';
      bool first = true;
      for (auto e: value.entries()) {
        if (!first) s += ',';
        first = false;
        s += '\'';
        escape(s, e.key(), true);
        s += "':";
        format(s, *e.value(), true);
      }
      s += '}';

    } else s += value.toString(); // A number
  }


  // One request.  The response or the timeout, whichever comes first,
  // completes it.  Holds itself through the HTTP callback until then.
  class Call : public RefCounted {
    Client::callback_t cb;
    HTTP::Client::RequestPtr pr;
    SmartPointer<Event::Event> timer;

  public:
    Call(Client::callback_t cb) : cb(cb) {}


    void send(HTTP::Client &client, const URI &uri, const string &data,
              const string &user, const string &password, unsigned timeout) {
      auto self = SmartPtr(this);
      auto responseCB = [self] (HTTP::Request &req) {
        self->complete(Response(req));
      };

      pr = client.call(uri, HTTP::Method::HTTP_POST, data, responseCB);

      // Credentials go in headers, never in the URL, which is logged
      auto &req = *pr->getRequest();
      req.outSet("X-ClickHouse-User", user);
      if (!password.empty()) req.outSet("X-ClickHouse-Key", password);

      auto pending = pr; // Failing to connect may complete the call
      try {
        pending->send();
      } catch (...) {
        pr.release(); // Break the reference cycle
        throw;
      }

      if (cb && timeout) {
        timer = client.getBase().newEvent(
          [this, timeout] {timedout(timeout);}, 0);
        timer->add(timeout);
      }
    }


    void timedout(unsigned timeout) {
      if (!cb) return; // Already completed
      auto self = SmartPtr(this);

      // Give up on the request.  Closing drops its callback.
      auto &conn = pr->getConnection();
      if (conn.isSet()) conn->close();

      complete(Response(SSTR("Timed out after " << timeout << "s"), true));
    }


    void complete(const Response &response) {
      if (!cb) return; // Already completed

      auto cb = this->cb;
      this->cb = 0;
      if (timer.isSet()) timer->del();
      pr.release();

      cb(response);
    }
  };
}


void Client::addOptions(Options &options) {
  options.pushCategory("ClickHouse");
  options.addTarget("clickhouse-url",  url,  "ClickHouse HTTP interface URL");
  options.addTarget("clickhouse-user", user, "ClickHouse user name");
  options.addTarget("clickhouse-pass", password,
                    "ClickHouse password")->setObscured();
  options.addTarget("clickhouse-reader-user", readerUser,
                    "ClickHouse user name for read-only queries.  Defaults "
                    "to clickhouse-user");
  options.addTarget("clickhouse-reader-pass", readerPassword,
                    "ClickHouse password for clickhouse-reader-user"
                    )->setObscured();
  options.addTarget("clickhouse-db",   database, "ClickHouse default database");
  options.addTarget("clickhouse-timeout", timeout,
                    "ClickHouse request timeout in seconds");
  options.popCategory();
}


void Client::query(const string &sql, const JSON::ValuePtr &params,
                   const JSON::ValuePtr &settings, callback_t cb,
                   bool readOnly) {
  URI uri = getURI(settings);

  if (params.isSet())
    for (auto e: params->entries())
      uri.set("param_" + e.key(), formatParam(*e.value()));

  send(uri, sql, cb, readOnly);
}


void Client::insert(const string &table, const JSON::Value &rows,
                    const JSON::ValuePtr &settings, callback_t cb) {
  URI uri = getURI(settings);
  uri.set("query", "INSERT INTO " + table + " FORMAT JSONEachRow");

  string data;
  for (auto &row: rows) data += row->toString(0, true) + "\n";

  send(uri, data, cb, false);
}


string Client::formatParam(const JSON::Value &value) {
  string s;
  format(s, value, false);
  return s;
}


URI Client::getURI(const JSON::ValuePtr &settings) const {
  URI uri(url);

  // A setting which resolved null is not sent
  if (settings.isSet())
    for (auto e: settings->entries())
      if (!e.value()->isNull()) uri.set(e.key(), e.value()->asString());

  // Every response parses the same way.  Waiting for the end of the query
  // makes an error set the HTTP status rather than truncate a 200.
  uri.set("default_format", "JSONCompact");
  uri.set("output_format_json_quote_64bit_integers", "0");
  uri.set("wait_end_of_query", "1");
  if (!database.empty()) uri.set("database", database);

  return uri;
}


void Client::send(const URI &uri, const string &data, callback_t cb,
                  bool readOnly) {
  if (!cb) THROW("Callback not set");

  // A separate login lets the server itself refuse writes from queries
  bool reader = readOnly && !readerUser.empty();
  SmartPtr(new Call(cb))->send(*client, uri, data,
                               reader ? readerUser : user,
                               reader ? readerPassword : password, timeout);
}
