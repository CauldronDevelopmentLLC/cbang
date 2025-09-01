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
  QueryDef(api, config), name(name), db(api.getTimeseriesDB().ns(name + "\0")),
  period(HumanDuration::parse(config->getAsString("period"))),
  event(db.getPool()->getEventBase().newEvent(
    this, &TimeseriesHandler::query, 0)) {

  if (name.empty()) THROW("Timeseries requires a name");
  if (!period) THROW("Timeseries period cannot be zero");

  // Query return type
  if (!config->hasString("return")) ret = "list";
  if (ret == "hlist") THROW("Timeseries return type cannot be 'hlist'");

  // Key
  if (config->has("key")) {
    key = config->get("key");

    if (!key->isList()) {
      auto l = config->createList();
      l->append(key);
      key = l;
    }

  } else {
    if (ret == "list")
      THROW("Timeseries with 'list' return type must provided a 'key'");

    key = config->createList(); // Empty key
  }

  load();
}


string TimeseriesHandler::resolveKey(const JSON::Value &dict) const {
  string result;

  for (auto it: *key) {
    if (!result.empty()) result += "\0";
    result += dict.getAsString(it->asString());
  }

  return result;
}


double TimeseriesHandler::getNext() const {
  auto   now  = Timer::now();
  double next = getTimePeriod(ceil(now)) + period - now;

  LOG_DEBUG(5, "Querying timeseries in " << next << " seconds "
    << " current=" << Time(getTimePeriod(ceil(now))).toString()
    << " period="  << period
    << " now="     << Time(now).toString());

  return next;
}


SmartPointer<Timeseries> TimeseriesHandler::get(
  const string &key, bool create) {

  auto it = series.find(key);
  if (it != series.end()) return it->second;
  if (!create) return 0;

  LOG_DEBUG(4, "Adding `" << name << "` timeseries with key `" << key << "`");
  return series[key] = new Timeseries(*this, key);
}



void TimeseriesHandler::action(const CtxPtr &ctx) {
  // Resolve variables
  auto resolver = ctx->getResolver();
  auto action   = resolver->selectString("args.action", "query");
  auto since    = resolver->selectTime("args.since", 0);
  auto maxCount = resolver->selectU64("args.max_count", 0);
  auto key      = resolveKey(*resolver->select("args"));

  // Get Timeseries
  auto ts = get(key);
  if (ts.isNull() && action == "unsubscribe") return;

  // Execute action
  auto cb = [this, ctx] (
    const SmartPointer<Exception> &err, const JSON::ValuePtr &data) {
      if (err.isNull()) ctx->reply(data);
      else ctx->reply(*err);
    };

  if (action == "query") return ts->query(since, maxCount, cb);

  auto ws = ctx->getWebsocket();
  if (ws.isSet())  {
    if (action == "subscribe")   return ws->subscribe(*ts, since, maxCount, cb);
    if (action == "unsubscribe") return ws->unsubscribe(*ts);
  }

  THROW("Unrecognized timeseries action '" << action << "'");
}


void TimeseriesHandler::load() {schedule();}


void TimeseriesHandler::schedule() {
  if (!event->isPending()) event->add(getNext());
}


void TimeseriesHandler::query(uint64_t time) {
  auto cb = [=] (HTTP::Status status, const JSON::ValuePtr &result) {
    schedule();

    if (status == HTTP::Status::HTTP_OK) {
      if (ret != "list") get("")->add(time, result);
      else {
        LOG_DEBUG(3, "Storing timeseries results " << name);

        for (auto e: *result) {
          string key = resolveKey(*e);

          for (auto it: *this->key)
            e->erase(it->asString());

          if (e->size() == 1) e = *e->begin();

          get(key)->add(time, e);
        }

        LOG_DEBUG(3, "Done " << name);
      }
    }
  };

  LOG_DEBUG(3, "Querying timeseries " << name);
  LOG_DEBUG(5, "querying: " << sql);

  query(sql, cb);
}


void TimeseriesHandler::query() {query(getTimePeriod(Time::now()));}


bool TimeseriesHandler::operator()(const CtxPtr &ctx) {
  action(ctx);
  return true;
}