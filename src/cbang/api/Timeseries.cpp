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

#include "Timeseries.h"
#include "Subscriber.h"
#include "API.h"
#include "Resolver.h"

#include <cbang/api/handler/TimeseriesHandler.h>
#include <cbang/json/Reader.h>
#include <cbang/log/Logger.h>

#include <cmath>

using namespace std;
using namespace cb;
using namespace cb::API;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "TS:" << handler.name << ":" << key << ":"
#define TIME_FMT "%Y%m%d%H%M%S"


Timeseries::Timeseries(TimeseriesHandler &handler, const string &key,
  const string &sql) : handler(handler), key(key), sql(sql),
  db(handler.db.ns("data:" + key + ":")),
  event(db.getPool()->getEventBase().newEvent(this, &Timeseries::query, 0)) {}


void Timeseries::load(uint64_t since, unsigned maxResults, const cb_t &cb) {
  auto _cb = [this, cb] (
    const EventLevelDB::Status &status,
    const SmartPointer<EventLevelDB::results_t> &results) {
    if (!status.isOk()) return cb(status.getException(), 0);

    JSON::ValuePtr data = new JSON::List;

    for (auto &result: *results) {
      auto time = Time::parse(result.first, TIME_FMT);
      data->append(makeEntry(time, JSON::Reader::parse(result.second)));
    }

    LOG_DEBUG(5, data->size() << " results");

    cb(0, data);
  };

  string last = since ? Time(since).toString(TIME_FMT) : "00000000000000";
  LOG_DEBUG(5, "getting max " << maxResults << " results since " << last);
  db.range(_cb, "99999999999999", last, true, 0, maxResults);

  requested();
  schedule();
}


SmartPointer<Subscriber> Timeseries::subscribe(
  uint64_t id, uint64_t since, unsigned maxResults, const cb_t &cb) {

  auto it = subscribers.find(id);
  if (it != subscribers.end() && it->second.isSet())
    THROWX("Timeseries already has subscriber with id " << id,
      HTTP::Status::HTTP_CONFLICT);

  auto subscriber = SmartPtr(new Subscriber(cb, this, id));
  subscribers[id] = subscriber; // Save weak pointer

  auto _cb = [this, cb, id] (const SmartPointer<Exception> &err,
    const JSON::ValuePtr &results) {

    auto it = subscribers.find(id);
    if (it != subscribers.end() && it->second.isSet()) {
      if (err.isNull()) it->second->first(results);
      else LOG_WARNING("Error loading data: " << *err);
    }
  };

  load(since, maxResults, _cb);

  return subscriber;
}


void Timeseries::unsubscribe(uint64_t id) {
  if (!subscribers.erase(id))
    THROWX("Timeseries does not have subscriber with id " << id,
      HTTP::Status::HTTP_NOT_FOUND);
}


void Timeseries::save() {
  if (!dirty) return;
  dirty = false;

  JSON::Dict state;

  state.insert("sql", sql);
  state.insert("last-request", lastRequest);
  if (lastResult.isSet()) state.insert("last-result", lastResult);

  handler.db.set("state:" + key, state.toString());
}


void Timeseries::load() {
  auto cb = [=] (bool success, const string &value) {
    if (success) {
      LOG_DEBUG(5, "state=" << value);
      auto state  = JSON::Reader::parse(value);
      lastRequest = state->getU64("last-request", 0);
      if (state->has("last-result")) lastResult = state->get("last-result");
    }

    schedule();
  };

  handler.db.get("state:" + key, "{}", cb);
}


void Timeseries::requested() {
  lastRequest = Time::now();
  dirty = true;
}


void Timeseries::setLastResult(const JSON::ValuePtr &last) {
  lastResult = last;
  dirty = true;
}


string Timeseries::getKey(uint64_t ts) const {
  return Time(handler.getTimePeriod(ts)).toString(TIME_FMT);
}


void Timeseries::query(uint64_t ts) {
  auto cb = [=] (HTTP::Status status, const JSON::ValuePtr &result) {
    if (status == HTTP::Status::HTTP_OK) {
      LOG_DEBUG(5, "result: " << result->toString() << " last: "
        << (lastResult.isSet() ? lastResult->toString() : "<null>"));

      // Don't record if result is unchanged
      if (lastResult.isNull() || *lastResult != *result) {
        setLastResult(result);

        db.set(getKey(ts), result->toString());

        auto entry = makeEntry(ts, result);
        for (auto p: subscribers)
          if (p.second.isSet()) p.second->next(entry);
      }

      if (!subscribers.empty()) requested();
      save();
    }

    schedule();
  };

  LOG_DEBUG(5, "querying: " << sql);

  handler.query(sql, cb);
}


void Timeseries::query() {query(handler.getTimePeriod(Time::now()));}


void Timeseries::schedule() {
  if (event->isPending()) return;
  auto next = handler.getNext(lastRequest);
  if (next) event->add(next);
}


JSON::ValuePtr Timeseries::makeEntry(uint64_t ts, const JSON::ValuePtr &value) {
  JSON::ValuePtr d = new JSON::Dict;
  d->insert("value", value);
  d->insert("time",  Time(ts).toString());
  return d;
}
