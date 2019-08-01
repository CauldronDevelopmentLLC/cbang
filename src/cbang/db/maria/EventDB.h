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

#include "DB.h"

#include <cbang/event/Base.h>

#include <functional>

namespace cb {
  namespace JSON {class Value;}

  namespace MariaDB {
    class EventDB : public DB {
      Event::Base &base;

    public:
      typedef enum {
        EVENTDB_ERROR,
        EVENTDB_BEGIN_RESULT,
        EVENTDB_ROW,
        EVENTDB_END_RESULT,
        EVENTDB_RETRY,
        EVENTDB_DONE,
      } state_t;


      template <class T> struct Callback {
        typedef void (T::*member_t)(state_t);
      };
      typedef std::function<void (state_t)> callback_t;

      EventDB(Event::Base &base, st_mysql *db = 0);

      unsigned getEventFlags() const;

      void newEvent(Event::Base::callback_t cb) const;
      void renewEvent(Event::Event &e) const;
      void addEvent(Event::Event &e) const;

      void callback(callback_t cb);
      void connect(callback_t cb,
                   const std::string &host = "localhost",
                   const std::string &user = "root",
                   const std::string &password = std::string(),
                   const std::string &dbName = std::string(),
                   unsigned port = 3306,
                   const std::string &socketName = std::string(),
                   flags_t flags = FLAG_NONE);

      template <class T>
      void connect(T *obj, typename Callback<T>::member_t member,
                   const std::string &host = "localhost",
                   const std::string &user = "root",
                   const std::string &password = std::string(),
                   const std::string &dbName = std::string(),
                   unsigned port = 3306,
                   const std::string &socketName = std::string(),
                   flags_t flags = FLAG_NONE) {
        using namespace std::placeholders;
        connect(std::bind(member, obj, _1), host, user, password, dbName, port,
                socketName, flags);
      }


      void query(callback_t cb, const std::string &s,
                 const SmartPointer<const JSON::Value> &dict = 0);

      template <class T>
      void query(T *obj, typename Callback<T>::member_t member,
                 const std::string &s,
                 const SmartPointer<const JSON::Value> &dict = 0) {
        using namespace std::placeholders;
        query(std::bind(member, obj, _1), s, dict);
      }

      // From DB
      using DB::connect;
      using DB::query;
    };
  }
}
