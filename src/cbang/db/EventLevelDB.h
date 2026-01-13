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

#pragma once

#include <cbang/config.h>

#ifdef HAVE_LEVELDB

#include "LevelDB.h"

#include <cbang/SmartPointer.h>
#include <cbang/event/ConcurrentPool.h>

#include <functional>


namespace cb {
  class EventLevelDB : public LevelDB {
    SmartPointer<Event::ConcurrentPool> pool;
    int priority = 0;

  public:
    class Status {
      bool success = true;
      SmartPointer<Exception> e;

      public:
        Status() {}
        Status(const SmartPointer<Exception> &e) : success(false), e(e) {}

        bool isOk() const {return success;}
        const SmartPointer<Exception> &getException() const {return e;}
    };

    EventLevelDB() {}


    EventLevelDB(
      const LevelDB &db, const SmartPointer<Event::ConcurrentPool> &pool,
      int priority = 0) : LevelDB(db), pool(pool), priority(priority) {}


    EventLevelDB(const SmartPointer<Event::ConcurrentPool> &pool,
      const SmartPointer<LevelDB::Comparator> &comparator = 0,
      int priority = 0) :
      LevelDB(comparator), pool(pool), priority(priority) {}


    EventLevelDB(const std::string &name,
      const SmartPointer<Event::ConcurrentPool> &pool,
      const SmartPointer<LevelDB::Comparator> &comparator = 0,
      int priority = 0) :
      LevelDB(name, comparator), pool(pool), priority(priority) {}


    const SmartPointer<Event::ConcurrentPool> &getPool() const {return pool;}


    int getPriority() const {return priority;}
    void setPriority(int priority) {this->priority = priority;}


    EventLevelDB ns(const std::string &name) {
      return EventLevelDB(LevelDB::ns(name), pool, priority);
    }


    EventLevelDB snapshot() {
      return EventLevelDB(LevelDB::snapshot(), pool, priority);
    }


    void has(const std::string &key, std::function<void (bool, bool)> cb,
      int options = 0) const {

      SmartPointer<bool> result = new bool(false);
      auto run  = [=] () {*result = LevelDB::has(key, options);};
      auto done = [=] (bool success) {if (cb) cb(success, *result);};
      pool->submit(run, done, priority);
    }


    void get(const std::string &key,
      std::function<void (bool, const std::string &)> cb,
      int options = 0) const {

      SmartPointer<std::string> result = new std::string;
      auto run  = [=] () {*result = LevelDB::get(key, options);};
      auto done = [=] (bool success) {if (cb) cb(success, *result);};
      pool->submit(run, done, priority);
    }


    void get(const std::string &key, const std::string &defaultValue,
      std::function<void (bool, const std::string &)> cb,
      int options = 0) const {

      SmartPointer<std::string> result = new std::string;
      auto run  = [=] () {*result = LevelDB::get(key, defaultValue, options);};
      auto done = [cb, result] (bool success) {if (cb) cb(success, *result);};
      pool->submit(run, done, priority);
    }


    using results_t = std::vector<std::pair<std::string, std::string>>;
    using range_cb_t = std::function<void (
      const Status &status, const SmartPointer<results_t> &)>;

    void range(range_cb_t cb, const std::string &first = std::string(),
      const std::string &last = std::string(), bool reverse = false,
      int options = 0, unsigned maxResults = 1000) const {

      auto results = SmartPtr(new results_t);

      auto query = [=] () {
        LevelDB::Iterator it = iterator(options);

        if (first.empty()) {
          if (reverse) it.last();
          else it.first();

        } else {
          it.seek(first);

          if (reverse) {
            if (it.valid()) {
              if (compare(first, it.key()) < 0) it--;
            } else it.last();
          }
        }

        for (unsigned i = 0; it.valid(); reverse ? it-- : it++) {
          if (maxResults == i++) break;
          if (!last.empty() && 0 <= compare(last, it.key())) break;
          results->push_back(results_t::value_type(it.key(), it.value()));
        }
      };

      auto success = [=] () {cb(Status(), results);};
      auto error = [=] (const Exception &e) {cb(Status(new Exception(e)), 0);};

      pool->submit(priority, query, success, error);
    }


    void commit(const SmartPointer<Batch> &batch,
      std::function<void (bool)> cb, int options = 0) {
      pool->submit([=] () {batch->commit(options);}, cb, priority);
    }


    void compact(std::function<void (bool)> cb,
      const std::string &begin = std::string(),
      const std::string &end   = std::string()) {

      pool->submit([=] {LevelDB::compact(begin, end);}, cb, priority);
    }
  };
}

#endif // HAVE_LEVELDB
