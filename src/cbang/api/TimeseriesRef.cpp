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

#include "TimeseriesRef.h"
#include "Timeseries.h"
#include "API.h"
#include "Resolver.h"

using namespace std;
using namespace cb;
using namespace cb::API;


TimeseriesRef::TimeseriesRef(API &api, const JSON::ValuePtr &config) :
  api(api), category(api.getCategory()),
  name(config->getString("timeseries", "")),
  sinceArg(config->getAsString("since_arg", "args.since")),
  maxCountArg(config->getAsString("max_count_arg", "args.max_count")) {

  if (name.empty()) {
    def = SmartPtr(new TimeseriesDef(api, config));
    api.addTimeseries(def->getName(), def);
  }
}


uint64_t TimeseriesRef::getSince(const ResolverPtr &resolver) const {
  auto sinceVal = resolver->select(sinceArg);
  return sinceVal.isNull() ? 0 : Time::parse(sinceVal->asString());
}


unsigned TimeseriesRef::getMaxCount(const ResolverPtr &resolver) const {
  auto maxCountVal = resolver->select(maxCountArg);
  return maxCountVal.isNull() ? 0 : maxCountVal->getU32();
}


SmartPointer<TimeseriesDef> TimeseriesRef::getDef(
  const ResolverPtr &resolver) const {
  if (def.isSet()) return def;
  string _name = (category.empty() ? "" : category + ".") + name;
  return api.getTimeseries(resolver->resolve(_name));
}


SmartPointer<Timeseries> TimeseriesRef::getTS(
  const ResolverPtr &resolver, bool create) const {
  return getDef(resolver)->get(resolver, create);
}


void TimeseriesRef::subscribe(
  const WebsocketPtr &ws, const ResolverPtr &resolver) {
  auto ts = getTS(resolver, true);
  LOG_DEBUG(3, "Websocket id=" << ws->getID() << " subscribing to timeseries "
    << ts->getKey());
  ws->subscribe(*ts, getSince(resolver), getMaxCount(resolver));
}


void TimeseriesRef::unsubscribe(
  const WebsocketPtr &ws, const ResolverPtr &resolver) {
  auto ts = getTS(resolver, false);
  if (ts.isNull())
    THROWX("Timeseries not found", HTTP::Status::HTTP_NOT_FOUND);
  ws->unsubscribe(*ts);
}


void TimeseriesRef::query(const ResolverPtr &resolver, cb_t cb) {
  getTS(resolver, true)->get(getSince(resolver), getMaxCount(resolver), cb);
}
