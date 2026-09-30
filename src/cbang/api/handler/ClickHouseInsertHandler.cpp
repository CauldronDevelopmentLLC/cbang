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

#include "ClickHouseInsertHandler.h"

#include <cbang/api/API.h>
#include <cbang/api/Resolver.h>
#include <cbang/api/arg/ArgDict.h>
#include <cbang/db/clickhouse/Client.h>
#include <cbang/json/Dict.h>
#include <cbang/json/List.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::API;


namespace {
  bool isIdentifier(const string &s) {
    if (s.empty() || isdigit((unsigned char)s[0])) return false;

    for (char c: s)
      if (!isalnum((unsigned char)c) && c != '_') return false;

    return true;
  }


  bool isTableName(const string &name) {
    auto dot = name.find('.');
    if (dot == string::npos) return isIdentifier(name);
    return isIdentifier(name.substr(0, dot)) &&
      isIdentifier(name.substr(dot + 1));
  }

}


ClickHouseInsertHandler::ClickHouseInsertHandler(
  API &api, const JSON::ValuePtr &config) : api(api) {
  auto insert = config->get("clickhouse-insert");
  if (!insert->isDict()) THROW("'clickhouse-insert' must be a dictionary");

  table   = insert->getString("table", "");
  maxRows = insert->getU32("max-rows", 10000);
  wait    = insert->getBoolean("wait", true);

  // Spliced into the INSERT statement
  if (!isTableName(table))
    THROW("Invalid ClickHouse insert table '" << table
          << "', expected [db.]table");

  if (insert->has("row")) row = new ArgDict(api, insert->get("row"));

  if (insert->has("set")) {
    set = insert->get("set");
    if (!set->isDict()) THROW("ClickHouse insert 'set' must be a dictionary");
  }

  if (insert->has("settings")) {
    settings = insert->get("settings");
    if (!settings->isDict())
      THROW("ClickHouse insert 'settings' must be a dictionary");
  }
}


void ClickHouseInsertHandler::operator()(const CtxPtr &ctx, const Cont &next) {
  ctx->errorHandler([&] {
    auto rows = getRows(ctx);
    unsigned count = rows->size();

    auto reply = [ctx, count] () {
      JSON::ValuePtr result = new JSON::Dict;
      result->insert("rows", count);
      ctx->reply(result);
    };

    if (!count) return reply();

    // Nothing is buffered here.  Durability comes from ClickHouse, which with
    // wait replies once the rows are written, and from clients retrying.
    JSON::ValuePtr settings = new JSON::Dict;
    settings->insert("async_insert", 1);
    settings->insert("wait_for_async_insert", wait ? 1 : 0);
    settings->insert("date_time_input_format", "best_effort");
    settings->insert("input_format_skip_unknown_fields", 0);
    settings->insert("input_format_null_as_default", 1);

    if (this->settings.isSet()) {
      auto extra = this->settings->copy(true);
      ctx->getResolver()->resolve(*extra);
      settings->merge(*extra);
    }

    auto cb = [ctx, reply] (const ClickHouse::Response &res) {
      ctx->errorHandler([&] {
        if (res.isOk()) return reply();

        // Data ClickHouse rejects is the client's to fix, retrying would
        // fail again.  Clients should retry when ClickHouse is unavailable.
        HTTP::Status status = HTTP_INTERNAL_SERVER_ERROR;
        if (res.isTimeout() || !res.hasResponse())
          status = HTTP_SERVICE_UNAVAILABLE;
        else if (res.isDataError()) status = HTTP_BAD_REQUEST;

        string msg = res.getMessage();
        if (status == HTTP_INTERNAL_SERVER_ERROR) LOG_ERROR(msg);
        else LOG_WARNING(msg);

        JSON::ValuePtr err = new JSON::Dict;
        err->insert("error", msg);
        err->insert("code", (unsigned)status);
        ctx->reply(status, err);
      });
    };

    api.getClickHouse().insert(table, *rows, settings, cb);
  });
}


JSON::ValuePtr ClickHouseInsertHandler::getRows(const CtxPtr &ctx) const {
  // Parsed with the request's args
  auto body = ctx->getRequest().getJSONMessage();
  if (body.isNull())
    THROWX("A JSON request body is required", HTTP_BAD_REQUEST);

  // A single row or a list of rows
  JSON::ValuePtr rows = body;
  if (body->isDict()) {
    rows = new JSON::List;
    rows->append(body);

  } else if (!body->isList())
    THROWX("Request body must be a JSON object or a list of objects",
           HTTP_BAD_REQUEST);

  if (maxRows < rows->size())
    THROWX("Too many rows " << rows->size() << ", max-rows is " << maxRows,
           HTTP_REQUEST_ENTITY_TOO_LARGE);

  // Values set in every row, resolved once per request
  JSON::ValuePtr set;
  if (this->set.isSet()) {
    set = this->set->copy(true);
    ctx->getResolver()->resolve(*set);
  }

  JSON::ValuePtr result = new JSON::List;

  for (unsigned i = 0; i < rows->size(); i++) {
    auto row = rows->get(i);

    if (!row->isDict())
      THROWX("Row " << i << " is not a JSON object", HTTP_BAD_REQUEST);

    if (this->row.isSet()) {
      // Strict, except for fields which set overwrites anyway
      for (auto e: row->entries())
        if (!this->row->has(e.key()) && (set.isNull() || !set->has(e.key())))
          THROWX("Row " << i << ": Unknown field '" << e.key() << "'",
                 HTTP_BAD_REQUEST);

      try {
        row = (*this->row)(ctx, row);
      } catch (const Exception &e) {
        THROWX("Row " << i << ": " << e.getMessage(),
               e.getCode() ? e.getCode() : HTTP_BAD_REQUEST);
      }

    } else row = row->copy();

    if (set.isSet())
      for (auto e: set->entries()) row->insert(e.key(), e.value());

    result->append(row);
  }

  return result;
}
