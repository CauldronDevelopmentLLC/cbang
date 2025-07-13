/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "TimeseriesHandler.h"

#include <cbang/api/API.h>
#include <cbang/api/Timeseries.h>
#include <cbang/time/HumanDuration.h>
#include <cbang/time/Timer.h>
#include <cbang/openssl/Digest.h>
#include <cbang/http/Status.h>
#include <cbang/json/Reader.h>

using namespace std;
using namespace cb;
using namespace cb::API;


TimeseriesHandler::TimeseriesHandler(
  API &api, const string &name, const JSON::ValuePtr &config) :
  QueryDef(api, config), name(name),
  period (HumanDuration::parse(config->getAsString("period"))),
  timeout(HumanDuration::parse(config->getAsString("timeout", "0"))) {

  if (name.empty()) THROW("Timeseries requires a name");
  if (!period) THROW("Timeseries period cannot be zero");

  string trigger = config->getString("trigger", "request");
  if (trigger != "auto" && trigger != "request")
    THROW("Invalid timeseries trigger '" << trigger
      << "' must be 'auto' or 'request'");

  automatic = trigger == "auto";

  if (automatic && timeout)
    THROW("It does not make sense for a timeseries to be both automatic and "
      "have a timeout.");

  load();
}


TimeseriesHandler::TimeseriesHandler(API &api, const JSON::ValuePtr &config) :
  TimeseriesHandler(api, config->getString("name", ""), config) {}


double TimeseriesHandler::getNext(uint64_t last) const {
  auto now = Timer::now();

  if (automatic || (last && (!timeout || now < last + timeout))) {
    double next = getTimePeriod(ceil(now)) + period - now;

    LOG_DEBUG(5, "Querying timeseries in " << next << " seconds "
      << " current=" << Time(getTimePeriod(ceil(now))).toString()
      << " period=" << period
      << " now=" << Time(now).toString());

    return next;
  }

  return 0;
}


SmartPointer<Timeseries> TimeseriesHandler::get(
  const CtxPtr &ctx, bool create) {
  string sql = ctx->getResolver()->resolve(getSQL(), true);
  string key = Digest::base64(sql, "sha256");

  auto it = series.find(key);
  if (it != series.end()) return it->second;
  if (!create) return 0;

  auto timeseries = SmartPtr(new Timeseries(*this, key, sql));
  add(timeseries);

  return timeseries;
}


void TimeseriesHandler::add(const SmartPointer<Timeseries> &ts) {
  LOG_DEBUG(3, "Adding `" << name << "` timeseries with key `" << ts->getKey()
    << "` and SQL `" << ts->getSQL() << "`");
  series[ts->getKey()] = ts;
  ts->load();
}


void TimeseriesHandler::action(const CtxPtr &ctx) {
  auto resolver = ctx->getResolver();
  auto action   = resolver->selectString("args.action", "query");
  auto since    = resolver->selectTime("args.since", 0);
  auto maxCount = resolver->selectU64("args.max_count", 0);
  auto ts       = get(ctx, action != "unsubscribe");

  auto cb = [this, ctx] (
    const SmartPointer<Exception> &err, const JSON::ValuePtr &data) {
      if (err.isNull()) ctx->reply(data);
      else ctx->reply(*err);
    };

  if (action == "query") return ts->load(since, maxCount, cb);

  auto ws = ctx->getWebsocket();
  if (ws.isSet())  {
    if (action == "subscribe")   return ws->subscribe(*ts, since, maxCount, cb);
    if (action == "unsubscribe") return ws->unsubscribe(*ts);
  }

  THROW("Unrecognized timeseries action '" << action << "'");
}


void TimeseriesHandler::load() {
  db = api.getTimeseriesDB().ns(":" + name + ":");

  auto cb = [=] (const EventLevelDB::Status &status,
    const SmartPointer<EventLevelDB::results_t> &results) {

    if (!status.isOk()) {
      LOG_WARNING("Failed to load timeseries: " << *status.getException());
      return;
    }

    for (auto &result: *results) try {
      auto state = JSON::Reader::parse(result.second);
      add(new Timeseries(*this, result.first, state->getString("sql")));
    } CATCH_ERROR;
  };

  stateDB = db.ns("state:");
  stateDB.range(cb);
}


bool TimeseriesHandler::operator()(const CtxPtr &ctx) {
  action(ctx);
  return true;
}
