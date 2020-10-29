/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#pragma once

#include "Rate.h"

#include <cbang/Exception.h>
#include <cbang/json/Serializable.h>
#include <cbang/json/Sink.h>

#include <string>
#include <map>


namespace cb {
  class RateSet : public JSON::Serializable {
    const unsigned size;
    const unsigned period;

    typedef std::map<const std::string, Rate> rates_t;
    rates_t rates;

  public:
    RateSet(unsigned size = 60 * 5, unsigned period = 1) :
      size(size), period(period) {}


    Rate &getRate(const std::string &key) {
      return rates.insert(rates_t::value_type(key, Rate(size, period)))
        .first->second;
    }


    const Rate &getRate(const std::string &key) const {
      auto it = rates.find(key);
      if (it == rates.end()) CBANG_THROW("Rate '" << key << "' not in set");
      return it->second;
    }


    void reset() {
      for (auto it = rates.begin(); it != rates.end(); it++)
        it->second.reset();
    }


    bool has(const std::string &key) const {
      return rates.find(key) != rates.end();
    }


    double get(const std::string &key, uint64_t now = Time::now()) const {
      return getRate(key).get(now);
    }


    void event(const std::string &key, double value = 1,
               uint64_t now = Time::now()) {
      getRate(key).event(value, now);
    }


    void insert(JSON::Sink &sink, bool withTotals = false) const {
      for (auto it = rates.begin(); it != rates.end(); it++)
        if (!withTotals) sink.insert(it->first, it->second.get());
        else {
          sink.insertDict(it->first);
          sink.insert("rate", it->second.get());
          sink.insert("total", it->second.getTotal());
          sink.endDict();
        }
    }


    void write(JSON::Sink &sink, bool withTotals) const {
      sink.beginDict();
      insert(sink, withTotals);
      sink.endDict();
    }

    // From JSON::Serializable
    void write(JSON::Sink &sink) const {write(sink, false);}
  };
}
