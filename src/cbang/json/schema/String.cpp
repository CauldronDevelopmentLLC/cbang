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

#include "String.h"
#include "Pattern.h"
#include "IPv4Addr.h"
#include "IPv6Addr.h"
#include "URIPattern.h"

#include <limits>

using namespace std;
using namespace cb::JSON::Schema;

#define ATEXT_RE "[\\w!#$%&'*+-/=?^`{|}~]"
#define SUBDOMAIN_RE "[a-zA-Z0-9][a-zA-Z0-9-]*"
#define DOMAIN_RE SUBDOMAIN_RE "(\\." SUBDOMAIN_RE ")+"
#define EMAIL_RE ATEXT_RE "+(\\." ATEXT_RE "+)?@" DOMAIN_RE

#define DATE_RE "\\d{4}-\\d{2}-\\d{2}"
#define TIME_RE "\\d{2}:\\d{2}:\\d{2}(\\.\\d{1,6})?"
#define ZONE_RE "(Z|([+-]\\d{2}:\\d{2}))"
#define ISO8601_RE DATE_RE "T" TIME_RE ZONE_RE

#define DECBYTE_RE "\\d{1,3}"
#define IPV4_RE DECBYTE_RE "." DECBYTE_RE "." DECBYTE_RE "." DECBYTE_RE

#define HEX_RE "[\\da-fA-F]"
#define UUID_RE \
  HEX_RE "{8}-" HEX_RE "{4}-" HEX_RE "{4}-" HEX_RE "{4}-" HEX_RE "{12}"


namespace {
  unsigned getInt(
    const cb::JSON::Value &spec, const string &name, unsigned defaultVal) {
    auto it = spec.find(name);
    return it ? (int)(*it)->getU32() : defaultVal;
  }
}


String::String(const cb::JSON::Value &spec) :
  maxLength(getInt(spec, "maxLength", numeric_limits<unsigned>::max())),
  minLength(getInt(spec, "minLength", 0)) {

  auto patIt = spec.find("pattern");
  if (patIt) pattern = new Pattern((*patIt)->getString());

  auto formatIt = spec.find("format");
  if (formatIt) {
    string fmt = (*formatIt)->getString();

    if      (fmt == "date-time") format = new Pattern("^" ISO8601_RE "$");
    else if (fmt == "time")      format = new Pattern("^" TIME_RE    "$");
    else if (fmt == "date")      format = new Pattern("^" DATE_RE    "$");
    else if (fmt == "email")     format = new Pattern("^" EMAIL_RE   "$");
    else if (fmt == "hostname")  format = new Pattern("^" DOMAIN_RE  "$");
    else if (fmt == "ipv4")      format = new IPv4Addr;
    else if (fmt == "ipv6")      format = new IPv6Addr;
    else if (fmt == "uuid")      format = new Pattern("^" UUID_RE    "$");
    else if (fmt == "uri")       format = new URIPattern;
    else THROW("Unsupported JSON Schema format '" << fmt << "'");

    // Unsupported formats:
    //   duration, regex, idn-email, idn-hostname, uri-reference, iri,
    //   iri-reference, uri-template, json-pointer, relative-json-pointer
  }
}


bool String::match(const cb::JSON::Value &value) const {
  if (!value.isString()) return false;

  auto &s = value.getString();
  if (s.length() < minLength || maxLength < s.length()) return false;

  if (pattern.isSet() && !pattern->match(value)) return false;
  if (format.isSet() && !format->match(value)) return false;

  return true;
}
