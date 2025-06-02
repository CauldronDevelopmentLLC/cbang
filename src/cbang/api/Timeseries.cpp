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

#include <cbang/time/Timer.h>
#include <cbang/json/Reader.h>
#include <cbang/log/Logger.h>

#include <cmath>

using namespace std;
using namespace cb;
using namespace cb::API;

#define TIME_FMT "%Y%m%d%H%M%S"


Timeseries::Timeseries(const TimeseriesDefPtr &def, const string &key,
  const string &sql) : def(def), key(key), sql(sql),
  db(def->db.ns(":" + key + ":")),
  event(db.getPool()->getEventBase().newEvent(this, &Timeseries::query)) {}


void Timeseries::get(uint64_t since, unsigned maxResults, const cb_t &cb) {
  auto _cb = [this, cb] (
    const SmartPointer<EventLevelDB::results_t> &results) {
    if (results.isNull()) return cb(0);

    try {
      JSON::ValuePtr data = new JSON::List;

      for (auto &result: *results) {
        auto time = Time::parse(result.first, TIME_FMT);
        data->append(makeEntry(time, JSON::Reader::parse(result.second)));
      }

      cb(data);
      return;
    } CATCH_ERROR;

    cb(0);
  };

  string last = since ? Time(since).toString(TIME_FMT) : "00000000000000";
  db.range(_cb, "99999999999999", last, true, 0, maxResults);

  lastRequest = Time::now();
  schedule();
  save();
}


SmartPointer<Subscriber> Timeseries::subscribe(
  uint64_t id, uint64_t since, unsigned maxResults, const cb_t &cb) {

  if (subscribers.find(id) != subscribers.end())
    THROWX("Timeseries already has subscriber with id " << id,
      HTTP::Status::HTTP_CONFLICT);

  auto subscriber = SmartPtr(new Subscriber(cb, this, id));
  subscribers[id] = subscriber; // Save weak pointer

  auto _cb = [this, cb, id] (const JSON::ValuePtr &results) {
    auto it = subscribers.find(id);
    if (it != subscribers.end()) it->second->first(results);
  };

  get(since, maxResults, _cb);

  return subscriber;
}


void Timeseries::unsubscribe(uint64_t id) {
  if (!subscribers.erase(id))
    THROWX("Timeseries does not have subscriber with id " << id,
      HTTP::Status::HTTP_NOT_FOUND);
}


void Timeseries::save() {
  JSON::Dict state;
  state.insert("last-request", lastRequest);
  if (lastResult.isSet()) state.insert("last-result", lastResult);
  db.set("state", state.toString());
}


void Timeseries::load() {
  auto cb = [=] (bool success, const string &value) {
    if (success) {
      auto state  = JSON::Reader::parse(value);
      lastRequest = state->getU64("last-request", 0);
      if (state->has("last-result")) lastResult = state->get("last-result");
    }

    schedule();
  };

  db.get("state", "{}", cb);
}


string Timeseries::getKey(uint64_t ts) const {
  return Time(def->getTimePeriod(ts)).toString(TIME_FMT);
}


void Timeseries::query(uint64_t ts) {
  auto cb = [=] (HTTP::Status status, const JSON::ValuePtr &result) {
    if (status == HTTP::Status::HTTP_OK) {
      // Don't record if result is unchanged
      if (lastResult.isNull() || *lastResult != *result) {
        lastResult = result;

        db.set(getKey(ts), result->toString());

        auto entry = makeEntry(ts, result);

        for (auto p: subscribers)
          p.second->next(entry);
      }

      if (!subscribers.empty()) {lastRequest = Time::now(); save();}
    }

    schedule();
  };

  def->queryDef->query(sql, cb);
}


void Timeseries::query() {query(def->getTimePeriod(Time::now()));}


void Timeseries::schedule() {
  if (event->isPending()) return;

  auto now = Timer::now();

  if (def->automatic ||
    (lastRequest && (!def->timeout || now < lastRequest + def->timeout))) {
    auto next = def->getTimePeriod(ceil(now)) + def->period - now;
    event->add(next);

    LOG_DEBUG(5, "Querying timeseries in " << next << " seconds "
      << " current=" << Time(def->getTimePeriod(ceil(now))).toString()
      << " period=" << def->period
      << " now=" << Time(now).toString());
  }
}


JSON::ValuePtr Timeseries::makeEntry(
  uint64_t ts, const JSON::ValuePtr &value) {

  JSON::ValuePtr d = new JSON::Dict;
  d->insert("value", value);
  d->insert("time",  Time(ts).toString());
  return d;
}
