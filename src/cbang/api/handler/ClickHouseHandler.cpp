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

#include "ClickHouseHandler.h"

#include <cbang/api/API.h>
#include <cbang/api/Resolver.h>
#include <cbang/db/clickhouse/Client.h>
#include <cbang/json/Dict.h>
#include <cbang/json/List.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::API;


namespace {
  bool isReturnType(const string &name) {
    for (auto type: {"ok", "pass", "list", "hlist", "dict", "one", "bool",
                     "u64", "s64"})
      if (name == type) return true;
    return false;
  }


  // The same shape as a sql: query error
  JSON::ValuePtr errorJSON(HTTP::Status code, const string &msg = "") {
    JSON::ValuePtr err = new JSON::Dict;
    err->insert("error", msg.empty() ? code.toString() : msg);
    err->insert("code", (unsigned)code);
    return err;
  }


  JSON::ValuePtr rowDict(const JSON::Value &meta, const JSON::Value &row) {
    JSON::ValuePtr dict = new JSON::Dict;
    for (unsigned i = 0; i < meta.size() && i < row.size(); i++)
      dict->insert(meta.get(i)->getString("name"), row.get(i));
    return dict;
  }
}


ClickHouseHandler::ClickHouseHandler(API &api, const JSON::ValuePtr &config) :
  api(api), sql(String::trim(config->getString("clickhouse", ""))),
  ret(config->getString("return", "ok")), into(config->getString("into", "")),
  settings(config->get("settings", 0)),
  readonly(config->getBoolean("readonly", true)) {

  if (sql.empty()) THROW("ClickHouse query must have 'clickhouse' SQL");

  if (!isReturnType(ret))
    THROW("Unsupported ClickHouse query return type '" << ret << "'");

  if (!into.empty() && ret == "pass")
    THROW("Query cannot have both 'into' and 'return: pass'");

  if (settings.isSet() && !settings->isDict())
    THROW("ClickHouse 'settings' must be a dictionary");
}


void ClickHouseHandler::operator()(const CtxPtr &ctx, const Cont &next) {
  auto &resolver = *ctx->getResolver();

  JSON::ValuePtr params = new JSON::Dict;
  string sql = resolver.resolveClickHouse(this->sql, *params);

  // Queries only read unless configured otherwise.  Writes should go through
  // clickhouse-insert.
  JSON::ValuePtr settings =
    this->settings.isSet() ? this->settings->copy(true) : new JSON::Dict;
  resolver.resolve(*settings);
  if (readonly) settings->insert("readonly", 2);

  auto cb = [this, ctx, next] (const ClickHouse::Response &res) {
    ctx->errorHandler([&] {
      HTTP::Status status = HTTP_OK;
      auto result = getResult(res, status);

      // As with sql:, on success a ``return: pass`` query continues the chain
      // and an ``into:`` query captures its result and continues.
      if (status == HTTP_OK && (ret == "pass" || !into.empty())) {
        if (!into.empty() && result.isSet())
          ctx->getResolver()->set(into, result);
        next(ctx);

      } else ctx->reply(status, result);
    });
  };

  api.getClickHouse().query(sql, params, settings, cb, readonly);
}


JSON::ValuePtr ClickHouseHandler::getResult(
  const ClickHouse::Response &res, HTTP::Status &status) const {

  if (!res.isOk()) {
    status = res.isTimeout() ? HTTP_GATEWAY_TIME_OUT :
      res.hasResponse() ? HTTP_INTERNAL_SERVER_ERROR : HTTP_SERVICE_UNAVAILABLE;

    string msg = res.getMessage();
    if (status == HTTP_INTERNAL_SERVER_ERROR) LOG_ERROR(msg);
    else LOG_WARNING(msg);

    return errorJSON(status, msg);
  }

  auto meta = res.getMeta();
  auto data = res.getData();

  if (ret == "pass") return 0; // Discard any results

  if (ret == "list" || ret == "hlist") {
    JSON::ValuePtr list = new JSON::List;

    if (ret == "hlist" && data->size()) {
      JSON::ValuePtr header = new JSON::List;
      for (auto &col: *meta) header->append(col->get("name"));
      list->append(header);
    }

    for (auto &row: *data)
      if (ret == "hlist") list->append(row);
      else if (meta->size() == 1) list->append(row->get(0));
      else list->append(rowDict(*meta, *row));

    return list;
  }

  // The rest expect one row, of one column except for dict
  unsigned rows = data->size();
  if (!rows && ret == "ok") return 0;

  if (!rows) {
    status = HTTP_NOT_FOUND;
    return errorJSON(status);
  }

  if (ret == "ok" || 1 < rows || (ret != "dict" && meta->size() != 1)) {
    string msg =
      SSTR("DB row unexpected with query return type '" << ret << "'");
    LOG_ERROR(msg);
    status = HTTP_INTERNAL_SERVER_ERROR;
    return errorJSON(status, msg);
  }

  auto row = data->get(0);
  if (ret == "dict") return rowDict(*meta, *row);

  auto value = row->get(0);

  if (ret == "bool")
    return value->createBoolean(value->isString() ?
      String::parseBool(value->getString()) : value->toBoolean());

  if (ret == "u64")
    return value->create(value->isString() ?
      String::parseU64(value->getString()) : value->getU64());

  if (ret == "s64")
    return value->create(value->isString() ?
      String::parseS64(value->getString()) : value->getS64());

  return value; // one
}
