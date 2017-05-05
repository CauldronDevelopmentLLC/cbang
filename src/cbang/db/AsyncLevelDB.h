/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2017, Cauldron Development LLC
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

#include "LevelDB.h"

#include <cbang/SmartPointer.h>


namespace cb {
  class AsyncLevelDB : public LevelDB {
    SmartPointer<AsyncScheduler> scheduler;

  public:
    AsyncLevelDB(LevelDB &db, const SmartPointer<AsyncScheduler> &scheduler) :
      LevelDB(db), scheduler(scheduler) {}

    AsyncLevelDB(const SmartPointer<AsyncScheduler> &scheduler,
                 const SmartPointer<LevelDB::Comparator> &comparator = 0) :
      LevelDB(comparator), scheduler(scheduler) {}

    AsyncLevelDB(const std::string &name,
                 const SmartPointer<AsyncScheduler> &scheduler,
                 const SmartPointer<LevelDB::Comparator> &comparator = 0) :
      LevelDB(name, comparator), scheduler(scheduler) {}


    AsyncLevelDB ns(const std::string &name) {
      return AsyncLevelDB(LevelDB::ns(name), scheduler);
    }


    void open(const std::string &path, std::function<void ()> cb,
              int options = 0) {
      scheduler->add([&] () {LevelDB::open(path, options);}, cb);
    }


    void close(std::function<void ()> cb) {
      scheduler->add([this] () {LevelDB::close()}, cb);
    }


    bool has(const std::string &key, std::function<void (bool)> cb,
             int options = 0) const {
      bool result = false;

      scheduler->add([&] () {result = LevelDB::has(key, options);},
                     [&] () {if (cb) cb(result);});
    }


    std::string get(const std::string &key,
                    std::function<void (const std::string &)> cb,
                    int options = 0) const {
      std::string result;

      scheduler->add([&] () {result = LevelDB::get(key, options);},
                     [&] () {if (cb) cb(result);});
    }


    std::string get(const std::string &key,
                    const std::string &defaultValue,
                    std::function<void (const std::string &)> cb,
                    int options = 0) const {
      std::string result;

      scheduler->add([&] () {
          result = LevelDB::get(key, defaultValue, options);
        }, [&] () {if (cb) cb(result);});
    }


    void foreach(std::function<bool (const std::string &key,
                                     const std::string &value)> cb,
                 const std::string &seek = std::string(),
                 bool reverse = false, int options = 0,
                 int batchSize = 1000) const {
      LevelDB::Iterator it;
      bool first = true;

      typedef std::vector<std::pair<std::string, std::string> > results_t;
      results_t results(batchSize);

      auto batch = [&] () {
        if (first) {
          it = LevelDB::iterator(options);

          if (seek.empty()) {
            if (reverse) it.last();
            else it.first();
          } else it.seek(seek);

          first = false;
        }

        for (int i = 0; i < batchSize && it.valid(); reverse ? it-- : it++)
          results.push_back(results_t::value_type(it.key(), it.value()));
      };

      auto process = [&] () {
        for (unsigned i = 0; i < results.size(); i++)
          if (!cb(results[i].first, results[i].second)) return;
        results.clear();

        if (first || it.valid()) scheduler->add(batch, process);
      };

      process(); // Kick things off
    }


    void commit(Batch &batch, std::function<void ()> cb, int options = 0) {
      scheduler->add([&] () {batch.commit(options);}, cb);
    }


    void compact(std::function<void ()> cb,
                 const std::string &begin = std::string(),
                 const std::string &end = std::string()) {
      scheduler->add([&] () {LevelDB::compact(begin, end);}, cb);
    }
  };
}
