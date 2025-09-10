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
/*
  This class handles the storing and recalling of timeseries data.  Queries
  are made to the SQL DB and those results are stored as a timeseries in
  LevelDB.

  Timeseries data may either be a single value or a list of values keyed by
  one or more variables.  The key or keys are specified via the ``key``
  configuration option.  The key value(s) are removed from the timeseries
  data to save space.  If the resulting data is a dictionary with a single
  value then only the value is stored.

  Clients may subscribe to the timeseries via Websocket.  Subscribers will
  first receive a list of all the recent timeseries data limited by time
  and total number of results.  Subsequently, new results will be broadcast
  to all subscribers for as long as they stay connected.

  The last result is tracked in LevelDB.  If a new result is the same as the
  last result it is not added to the timeseries.  In this way, gaps in the time
  series are assumed to be repeats of the previous value.

  The "last" result is stored at key "\0".  If the timeseries is a list of
  values then "last" will be dictionary of key, value pairs, otherwise just the
  one value.  The "last" value is reloaded when the server is restarted before
  making further queries.

  The implementation is complicated because care is taken to not dominate the
  main thread when processing large lists of data.  A low priority event is
  used, repetitive operations are spread across multiple event callbacks and
  LevelDB writes are batched.
*/


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

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "TS:" << name << ":"
#define TIME_FMT "%Y%m%d%H%M%S"


TimeseriesHandler::TimeseriesHandler(
  API &api, const string &name, const JSON::ValuePtr &config) :
  QueryDef(api, config), name(name), db(api.getTimeseriesDB().ns(name + "\0"s)),
  period(HumanDuration::parse(config->getAsString("period"))),
  event(db.getPool()->getEventBase().newEvent(
    this, &TimeseriesHandler::process, 0)) {

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

  event->setPriority(7);

  schedule();
}


string TimeseriesHandler::resolveKey(const JSON::Value &dict) const {
  string result;

  for (auto it: *key) {
    if (!result.empty()) result += "\0"s;
    result += dict.getAsString(it->asString());
  }

  return result;
}


double TimeseriesHandler::getNext() const {
  auto   now  = Timer::now();
  double next = getTimePeriod(ceil(now)) + period - now;

  LOG_DEBUG(4, "Querying in " << next << " seconds "
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

  LOG_DEBUG(5, "Adding key `" << key << "`");
  return series[key] = new Timeseries(*this, key);
}


void TimeseriesHandler::schedule() {
  if (last.isNull() || results.isSet()) event->add(0);
  else event->add(getNext());
}


void TimeseriesHandler::process() {
  if (last.isNull()) {
    // Try to load the last value
    LOG_DEBUG(3, "Querying last value");

    auto cb = [this] (bool success, const string &value) {
      LOG_DEBUG(3, "Last value success=" << (success ? "true" : "false"));

      last = new JSON::Dict;
      schedule();

      try {
        if (success) last = JSON::Reader::parse(value);
      } catch (const Exception &e) {
        LOG_ERROR("Failed to parse last timeseries result: " << e);
      }
    };

    db.get("\0"s, cb);

  } else if (results.isSet()) {
    // Process a batch of results
    LOG_DEBUG(4, "Processing result batch");

    try {
      for (unsigned i = 0; i < 1000; i++) {
        if (it == results->end()) {
          if (batch.isSet()) {
            LOG_DEBUG(3, "Writing to DB " << name);

            auto cb = [batch = batch, last = last, name = name] () {
              batch->set("\0"s, last->toString());
              batch->commit();
              LOG_DEBUG(3, "Done " << name);
            };

            batch.release();
            db.getPool()->submit(0, cb);

          } else LOG_DEBUG(3, "No results, done " << name);

          results.release();
          break;
        }

        auto result = *it++;
        string key  = resolveKey(*result);

        // Don't store key data
        for (auto it: *this->key)
          result->erase(it->asString());

        // If there's only one key store just its value
        if (result->size() == 1) result = *result->begin();

        // Don't record if result is unchanged
        auto it = last->find(key);
        if (it != last->end() && **it == *result) continue;
        last->insert(key, result);

        if (batch.isNull()) batch = new LevelDB::Batch(db.batch());
        batch->set(key + "\0"s + resultsTimeKey, result->toString());
        get(key)->broadcast(resultsTime, result);
      }
    } catch (const Exception &e) {
      LOG_ERROR(e);
      LOG_DEBUG(3, "Failed " << name);
      results.release();
    }

    schedule();

  } else query(getTimePeriod(Time::now())); // Load next timeseries result
}


void TimeseriesHandler::query(uint64_t time) {
  auto cb = [=] (HTTP::Status status, const JSON::ValuePtr &results) {
    if (status == HTTP::Status::HTTP_OK) try {
      resultsTimeKey = Time(getTimePeriod(time)).toString(TIME_FMT);

      if (ret != "list") {
        if (*last != *results) {
          last = results;
          db.set("\0"s, last->toString());
          db.set("\0"s + resultsTimeKey, results->toString());
          get("")->broadcast(time, results);
        }

      } else {
        LOG_DEBUG(3, "Storing results");
        this->results = results;
        resultsTime   = time;
        it            = results->begin();
      }
    } CATCH_ERROR;

    schedule();
  };

  LOG_DEBUG(3, "Querying");
  LOG_DEBUG(5, "query: " << sql);

  query(sql, cb);
}


void TimeseriesHandler::action(const CtxPtr &ctx) {
  // Resolve variables
  auto resolver = ctx->getResolver();
  auto action   = resolver->selectString("args.action", "query");
  auto since    = resolver->selectTime("args.since", 0);
  auto maxCount = resolver->selectU64("args.max_count", 0);

  // Get Timeseries
  auto key = ret == "list" ? resolveKey(*resolver->select("args")) : "";
  auto ts  = get(key, action != "unsubscribe");
  if (ts.isNull()) return;

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


SmartPointer<MariaDB::EventDB> TimeseriesHandler::getDBConnection() const {
  auto db = QueryDef::getDBConnection();
  db->setPriority(8); // Lower priority to avoid blocking regular API requests
  return db;
}


bool TimeseriesHandler::operator()(const CtxPtr &ctx) {
  action(ctx);
  return true;
}