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

#include "TimeseriesDef.h"
#include "Timeseries.h"
#include "Resolver.h"
#include "API.h"

#include <cbang/time/HumanDuration.h>
#include <cbang/openssl/Digest.h>
#include <cbang/http/Status.h>

using namespace std;
using namespace cb;
using namespace cb::API;


TimeseriesDef::TimeseriesDef(
  API &api, const string &name, const JSON::ValuePtr &config) :
  api(api), name(name),
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

  // QueryDef
  if (config->hasString("query")) {
    if (config->has("sql"))
      THROW("Timeseries cannot define both 'query' and 'sql'");
    queryDef = api.getQuery(config->getString("query"));

  } else queryDef = new QueryDef(api, config);
}


TimeseriesDef::TimeseriesDef(API &api, const JSON::ValuePtr &config) :
  TimeseriesDef(
    api, config->getString("name", config->getString("query", "")), config) {}


SmartPointer<Timeseries> TimeseriesDef::get(
  const ResolverPtr &resolver, bool create) {

  string sql = resolver->resolve(queryDef->getSQL());
  string key = Digest::base64(sql, "sha256");

  auto it = series.find(key);
  if (it != series.end()) return it->second;
  if (!create) return 0;

  auto timeseries = SmartPtr(new Timeseries(this, key, sql));
  add(timeseries);

  return timeseries;
}


void TimeseriesDef::add(const SmartPointer<Timeseries> &ts) {
  LOG_DEBUG(3, "Adding `" << name << "` timeseries with key `" << ts->getKey()
    << "` and SQL `" << ts->getSQL() << "`");
  series[ts->getKey()] = ts;
  db.set("keys:" + ts->getKey(), ts->getSQL());
  ts->load();
}


void TimeseriesDef::load() {
  db = api.getTimeseriesDB().ns(":" + name + ":");
  dbKeys = db.ns("keys:");

  auto cb = [=] (const SmartPointer<EventLevelDB::results_t> &results) {
    try {
      for (auto &result: *results)
        add(new Timeseries(this, result.first, result.second));
    } CATCH_ERROR;
  };

  dbKeys.range(cb);
}
