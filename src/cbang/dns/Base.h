/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "Nameserver.h"
#include "Result.h"
#include "RequestResolve.h"
#include "RequestReverse.h"

#include <map>
#include <set>
#include <list>


namespace cb {
  namespace Event {class Base;}

  namespace DNS {
    class Base : public Error::Enum {
    public:
      typedef SmartPointer<Request> RequestPtr;

    protected:
      Event::Base &base;

      SmartPointer<Event::Event> pumpEvent;

      typedef std::vector<SmartPointer<Nameserver>> servers_t;
      servers_t servers;
      uint64_t lastSystemNSInit = 0;

      struct Entry {
        uint64_t expires  = 0;
        unsigned attempts = 0;
        unsigned inflight = 0;
        bool responded = false;
        SmartPointer<Result> result;
        std::list<RequestPtr> requests;

        bool isValid() const;
        void respond(const SmartPointer<Result> &result, uint64_t expires = 0);
      };

      typedef std::map<std::string, Entry> cache_t;
      cache_t cache;

      std::set<std::string>  active;
      std::list<std::string> pending;
      std::list<std::string> ready;

      SockAddr bindAddr;
      unsigned maxActive      = 512;
      unsigned queryTimeout   = 5;
      unsigned requestTimeout = 16;
      unsigned maxAttempts    = 3;
      unsigned maxFailures    = 16;

    public:
      Base(Event::Base &base);
      ~Base();

      Event::Base &getEventBase() {return base;}

      const SockAddr &getBindAddress   () const {return bindAddr;}
      unsigned        getMaxActive     () const {return maxActive;}
      unsigned        getQueryTimeout  () const {return queryTimeout;}
      unsigned        getRequestTimeout() const {return requestTimeout;}
      unsigned        getMaxAttempts   () const {return maxAttempts;}
      unsigned        getMaxFailures   () const {return maxFailures;}

      void setBindAddress   (const SockAddr &addr) {bindAddr       = addr;}
      void setMaxActive     (unsigned x)           {maxActive      = x;}
      void setQueryTimeout  (unsigned x)           {queryTimeout   = x;}
      void setRequestTimeout(unsigned x)           {requestTimeout = x;}
      void setMaxAttempts   (unsigned x)           {maxAttempts    = x;}
      void setMaxFailures   (unsigned x)           {maxFailures    = x;}

      void initSystemNameservers();
      bool hasNameserver(const SockAddr &addr) const;
      void addNameserver(const SmartPointer<Nameserver> &server);
      void addNameserver(const std::string &addr, bool system = false);
      void addNameserver(const SockAddr &addr, bool system = false);

      void add(const RequestPtr &req);
      RequestPtr resolve(const std::string &name, RequestResolve::callback_t cb,
        bool ipv6 = false);
      RequestPtr reverse(const SockAddr &addr, RequestReverse::callback_t cb);
      RequestPtr reverse(
        const std::string &addr, RequestReverse::callback_t cb);

      // Called by Nameserver
      void response(Type type, const std::string &request,
        const cb::SmartPointer<Result> &result, unsigned ttl);
      void schedule();

    private:
      static std::string makeID(Type type, const std::string &request);
      Entry &lookup(const std::string &id);
      void pump();
      void error(Entry &e, const std::string &id, Error error);
    };
  }
}
