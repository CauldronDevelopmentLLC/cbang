/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "DNSBase.h"

#include "Base.h"

#include <cbang/Exception.h>

#include <event2/dns.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


DNSBase::DNSBase(cb::Event::Base &base, bool initialize,
                 bool failRequestsOnExit) :
  dns(evdns_base_new
      (base.getBase(), initialize ? EVDNS_BASE_INITIALIZE_NAMESERVERS : 0)),
  failRequestsOnExit(failRequestsOnExit) {
  if (!dns) THROW("Failed to create DNSBase");
}


DNSBase::~DNSBase() {if (dns) evdns_base_free(dns, failRequestsOnExit);}


void DNSBase::initSystemNameservers() {
#ifdef _WIN32
  int ret = evdns_base_config_windows_nameservers(dns);
#else
  int ret =
    evdns_base_resolv_conf_parse(dns, DNS_OPTIONS_ALL, "/etc/resolv.conf");
#endif

  if (ret) THROW("Failed to initialize system nameservers: " << ret);
}


void DNSBase::addNameserver(const string &addr) {
  evdns_base_nameserver_ip_add(dns, addr.c_str());
}


void DNSBase::addNameserver(const IPAddress &ns) {
  evdns_base_nameserver_add(dns, htonl(ns.getIP()));
}


void DNSBase::setOption(const string &name, const string &value) {
  if (evdns_base_set_option(dns, name.c_str(), value.c_str()))
    THROW("Failed to set DNS option " << name << "=" << value);
}


SmartPointer<DNSRequest>
DNSBase::resolve(const string &name, callback_t cb, bool search) {
  return new DNSRequest(dns, name, cb, search);
}


SmartPointer<DNSRequest>
DNSBase::reverse(uint32_t ip, callback_t cb, bool search) {
  return new DNSRequest(dns, ip, cb, search);
}
