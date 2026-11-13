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

#include "Base.h"

#include <cbang/Catch.h>
#include <cbang/event/Base.h>
#include <cbang/event/Event.h>
#include <cbang/time/Time.h>
#include <cbang/os/SystemInfo.h>
#include <cbang/os/SystemUtilities.h>

#include <algorithm>

using namespace std;
using namespace cb;
using namespace cb::DNS;


bool Base::Entry::isValid() const {
  return result.isSet() && Time::now() < expires;
}


void Base::Entry::respond(
  const SmartPointer<Result> &result, uint64_t expires) {
  this->result  = result;
  this->expires = expires;
  responded = true;

  for (auto &req: requests)
    req->respond(result);

  requests.clear();
}


Base::Base(Event::Base &base) :
  base(base), pumpEvent(base.newEvent([this] {pump();}, 0)) {}

Base::~Base() {}


void Base::initSystemNameservers() {
  if (lastSystemNSInit && Time::now() - lastSystemNSInit < 5) return;
  lastSystemNSInit = Time::now();

  // Nameservers added by hand, with addNameserver() or CBANG_DNS, are all
  // that is used; the system configuration is then left alone.
  for (auto &server: servers)
    if (!server->isSystem()) return;

#ifdef DEBUG
  // Check for environment variable
  const char *s = SystemUtilities::getenv("CBANG_DNS");
  if (s) {
    vector<string> servers;
    String::tokenize(s, servers);

    if (servers.empty())
      THROW("Failed to parse `CBANG_DNS` environment variable");

    for (auto &server: servers)
      addNameserver(server);

    return;
  }
#endif // DEBUG

  // Read the nameservers the system is using now.  The system usually gives
  // no port and Nameserver then uses 53, so compare on the same terms.
  vector<SockAddr> addrs;
  SystemInfo::instance().getNameservers(addrs);
  for (auto &addr: addrs)
    if (!addr.getPort()) addr.setPort(53);
  sort(addrs.begin(), addrs.end());
  addrs.erase(unique(addrs.begin(), addrs.end()), addrs.end());

  if (servers.empty()) {
    for (auto &addr: addrs)
      TRY_CATCH_DEBUG(4, addNameserver(addr, true));

    systemNS = addrs;
    changedSystemNS.clear();
    return;
  }

  // An empty list is ignored so a transient failure cannot remove working
  // servers.  An unchanged list leaves the servers as they are, so servers
  // dropped by pump() after too many failures are not added back.
  if (addrs.empty() || addrs == systemNS) {
    changedSystemNS.clear();
    return;
  }

  // A changed list, e.g. after a VPN reconnect, is used only once two reads
  // agree.  While the network is being reconfigured the system can report a
  // partial list for a few seconds.
  if (addrs != changedSystemNS) {
    changedSystemNS = addrs;
    return;
  }

  LOG_DEBUG(4, "System nameservers changed");

  // Add the servers that are not in use.  This includes servers dropped by
  // pump() after too many failures: those failures were on the old network.
  for (auto &addr: addrs)
    if (!hasNameserver(addr))
      TRY_CATCH_DEBUG(4, addNameserver(addr, true));

  // Drop system nameservers the system no longer reports
  for (auto it = servers.begin(); it != servers.end();)
    if ((*it)->isSystem() &&
        !binary_search(addrs.begin(), addrs.end(), (*it)->getAddress())) {
      LOG_DEBUG(4, "Removing nameserver " << (*it)->getAddress());
      (*it)->stop();
      it = servers.erase(it);
    } else it++;

  systemNS = addrs;
  changedSystemNS.clear();
}


bool Base::hasNameserver(const SockAddr &addr) const {
  for (auto &server: servers)
    if (server->getAddress() == addr) return true;

  return false;
}


void Base::addNameserver(const SmartPointer<Nameserver> &server) {
  LOG_DEBUG(4, "Adding nameserver " << server->getAddress());

  if (hasNameserver(server->getAddress())) return;

  servers.push_back(server);
  schedule();
}


void Base::addNameserver(const string &addr, bool system) {
  addNameserver(SockAddr::parse(addr), system);
}


void Base::addNameserver(const SockAddr &addr, bool system) {
  addNameserver(new Nameserver(*this, addr, system));
}


void Base::add(const RequestPtr &req) {
  if (servers.empty()) initSystemNameservers();

  string id = makeID(req->getType(), req->toString());
  auto &e   = lookup(id);

  // Check cache
  if (e.isValid()) req->respond(e.result);
  else {
    e.requests.push_back(req);

    if (!e.inflight) {
      e.attempts = 0;
      pending.push_back(id);
      schedule();
    }
  }
}


RequestPtr Base::resolve(
  const string &name, RequestResolve::callback_t cb, bool ipv6) {
  auto req = SmartPtr(new RequestResolve(*this, name, cb, ipv6));

  SockAddr addr;
  if (addr.readUnix(name) ||
      (ipv6 && addr.readIPv6(name)) || (!ipv6 && addr.readIPv4(name))) {
    // An already resolved literal address (IPv4/IPv6/Unix); no lookup needed
    auto result = SmartPtr(new Result(DNS_ERR_NOERROR));
    result->addrs.push_back(addr);
    req->respond(result);

  } else add(req);

  return req;
}


RequestPtr Base::reverse(const SockAddr &addr, RequestReverse::callback_t cb) {
  auto req = SmartPtr(new RequestReverse(*this, addr, cb));
  add(req);
  return req;
}


RequestPtr Base::reverse(const string &addr, RequestReverse::callback_t cb) {
  return reverse(SockAddr::parse(addr), cb);
}


void Base::response(Type type, const std::string &request,
                    const SmartPointer<Result> &result, unsigned ttl) {
  string id = makeID(type, request);
  auto &e   = lookup(id);

  LOG_DEBUG(5, "DNS: response " << result->error << " to " << id << " with "
            << e.requests.size() << " waiting requests, " << e.attempts
            << " attempts, " << e.inflight << " inflight, responded="
            << e.responded);

  if (!--e.inflight) active.erase(id);
  if (e.responded) return;

  if (result->error && result->error != DNS_ERR_NOTEXIST) {
    if (e.inflight) return; // Wait for other responses

    // Retry
    if (e.attempts < maxAttempts) {
      pending.push_front(id);
      return schedule();
    }
  }

  // Respond to any active requests
  e.respond(result, Time::now() + ttl);
}


void Base::schedule() {pumpEvent->activate();}


string Base::makeID(Type type, const string &request) {
  return String::toLower(string(type.toString()) + ":" + request);
}


Base::Entry &Base::lookup(const string &id) {
  auto it = cache.find(id);
  if (it != cache.end()) return it->second;
  return cache.insert(cache_t::value_type(id, Entry())).first->second;
}


void Base::pump() {
  // Follow the system nameservers, at most once every five seconds.  Servers
  // are added and removed here, and not from add(), because this runs from an
  // event; no Nameserver is running on the stack when one is removed.
  TRY_CATCH_ERROR(initSystemNameservers());

  // Drop failed Nameservers
  for (auto it = servers.begin(); it != servers.end();)
    if ((*it)->isSystem() && maxFailures < (*it)->getFailures()) {
      (*it)->stop();
      it = servers.erase(it);
    } else it++;

  // Send events
  while (!pending.empty()) {
    auto id = pending.front();
    pending.pop_front();

    if (active.find(id) == active.end()) {
      auto &e = lookup(id); // Get cache entry

      // Drop any canceled requests
      for (auto it = e.requests.begin(); it != e.requests.end();)
        if ((*it)->isCanceled()) it = e.requests.erase(it);
        else it++;

      if (e.requests.empty())   error(e, id, DNS_ERR_NOERROR);
      else if (servers.empty()) error(e, id, DNS_ERR_NOSERVER);
      else {
        // Transmit request
        try {
          auto &req = *e.requests.front();

          e.responded = false;
          e.inflight  = 0;
          for (auto &server: servers)
            if (server->transmit(req.getType(), req.toString()))
              e.inflight++;

          if (!e.inflight) error(e, id, DNS_ERR_NOSERVER);
          else {
            active.insert(id);
            e.attempts++;
          }
          continue;
        } CATCH_DEBUG(4);

        // Failed request
        error(e, id, DNS_ERR_BADREQ);
      }
    }
  }
}


void Base::error(Entry &e, const std::string &id, Error error) {
  e.respond(new Result(error));
  cache.erase(id);
}
