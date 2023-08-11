/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "URI.h"

#include <cbang/String.h>
#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <sstream>
#include <cctype>
#include <cstring>

using namespace cb;
using namespace std;


#define DIGIT_CHARS        "1234567890"
#define HEX_CHARS          DIGIT_CHARS "abcdefABCDEF"
#define LOWER_CHARS        "abcdefghijklmnopqrstuvwxyz"
#define UPPER_CHARS        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ALPHA_CHARS        LOWER_CHARS UPPER_CHARS
#define ALPHANUMERIC_CHARS ALPHA_CHARS DIGIT_CHARS
#define RESERVED_CHARS     ";/?:@&=+$,"
#define UNRESERVED_CHARS   ALPHANUMERIC_CHARS "-_.!~*'()"
#define USER_PASS_CHARS    UNRESERVED_CHARS ";&=+$,"
#define NAME_CHARS         UNRESERVED_CHARS ";/?:@+$,"
#define VALUE_CHARS        NAME_CHARS "="
#define PATH_SEGMENT_CHARS UNRESERVED_CHARS ":@&=+$,"
#define HOST_CHARS         ALPHANUMERIC_CHARS "-."
#define SCHEME_CHARS       ALPHANUMERIC_CHARS "+-."


namespace {
  int hexToInt(char x) {
    return isdigit(x) ? x - '0' : ((islower(x) ? x - 'a' : x - 'A') + 10);
  }


  char nibblesToChar(char a, char b) {
    return (char)((hexToInt(a) << 4) + hexToInt(b));
  }


  char match(const char *&s, char c) {
    if (*s != c) THROW("Expected '" << c << "'");
    s++;
    return c;
  }


  bool consume(const char *&s, char c) {
    if (*s == c) s++;
    else return false;
    return true;
  }


  bool contains(const char *s, char c) {return c && strchr(s, c);}
}


const char *URI::DEFAULT_UNESCAPED = UNRESERVED_CHARS "/";


URI::URI(const string &scheme, const IPAddress &addr, const string &path) :
  scheme(scheme), host(addr.getHost()), port(addr.getPort()) {setPath(path);}


unsigned URI::getPort() const {
  if (port || scheme.empty()) return port;

  unsigned port = portFromScheme(scheme);
  if (port) return port;

  THROW("Unknown scheme '" << String::escapeC(scheme) << "' and port not set");
}


string URI::getEscapedPath() const {
  string path;
  for (unsigned i = 0; i < pathSegs.size(); i++)
    path += "/" + encode(pathSegs[i], PATH_SEGMENT_CHARS);
  return path;
}


string URI::getExtension() const {
  if (pathSegs.empty()) return "";
  size_t ptr = pathSegs.back().find_last_of('.');
  return (ptr == string::npos) ? "" : pathSegs.back().substr(ptr + 1);
}


const string URI::getQuery() const {
  ostringstream str;
  writeQuery(str);
  return decode(str.str());
}


void URI::setPath(const string &_path) {
  string path = encode(_path);
  const char *s = path.c_str();
  pathSegs.clear();
  parsePath(s);
  if (*s) THROW("Invalid path: " + path);
}


void URI::setQuery(const string &query) {setQuery(query.c_str());}


void URI::setQuery(const char *query) {
  StringMap::clear(); // Reset query string vars
  const char *s = query;

  try {
    parseQuery(s);
  } catch (const Exception &e) {
    THROW("Failed to parse URI query '" << String::escapeC(query)
           << "' at char " << (s - query) << ": " << e.getMessage());
  }
}


unsigned URI::portFromScheme(const string &scheme) {
  if (scheme == "ftp")       return 21;
  if (scheme == "ssh")       return 22;
  if (scheme == "telnet")    return 23;
  if (scheme == "smtp")      return 25;
  if (scheme == "domain")    return 53;
  if (scheme == "tftp")      return 69;
  if (scheme == "gopher")    return 70;
  if (scheme == "finger")    return 79;
  if (scheme == "http")      return 80;
  if (scheme == "ws")        return 80;
  if (scheme == "pop2")      return 109;
  if (scheme == "pop")       return 110;
  if (scheme == "pop3")      return 110;
  if (scheme == "auth")      return 113;
  if (scheme == "sftp")      return 115;
  if (scheme == "nntp")      return 119;
  if (scheme == "ntp")       return 123;
  if (scheme == "imap")      return 143;
  if (scheme == "snmp")      return 161;
  if (scheme == "irc")       return 194;
  if (scheme == "wais")      return 210;
  if (scheme == "imap3")     return 220;
  if (scheme == "ldap")      return 389;
  if (scheme == "https")     return 443;
  if (scheme == "wss")       return 443;
  if (scheme == "uucp")      return 540;
  if (scheme == "nntps")     return 563;
  if (scheme == "ldaps")     return 636;
  if (scheme == "ftps-data") return 989;
  if (scheme == "telnets")   return 992;
  if (scheme == "imaps")     return 993;
  if (scheme == "pop3s")     return 995;
  if (scheme == "suucp")     return 4031;
  return 0;
}


bool URI::schemeRequiresSSL(const std::string &scheme) {
  // TODO not exhaustive
  return
    scheme == "sftp"      ||
    scheme == "https"     ||
    scheme == "wss"       ||
    scheme == "nntps"     ||
    scheme == "ldaps"     ||
    scheme == "ftps-data" ||
    scheme == "telnets"   ||
    scheme == "imaps"     ||
    scheme == "pop3s"     ||
    scheme == "suucp";
}


bool URI::schemeRequiresSSL() const {return schemeRequiresSSL(scheme);}


void URI::clear() {
  StringMap::clear();
  scheme.clear();
  host.clear();
  port = 0;
  path.clear();
  pathSegs.clear();
  user.clear();
  pass.clear();
}


void URI::normalize() {
  // NOTE, last empty seg denotes a trailing /, do not remove it
  for (auto it = pathSegs.begin(); it != pathSegs.end();)
    if (*it == "..") {
      if (it == pathSegs.begin()) THROW("Invalid path, '..' with no parent");
      pathSegs.erase(it - 1);
      it = pathSegs.erase(it);

    } else if (*it == "." || (it->empty() && (it + 1) != pathSegs.end()))
      it = pathSegs.erase(it);
    else it++;

  path = "/" + String::join(pathSegs, "/");
}


void URI::read(const string &uri) {read(uri.c_str());}


void URI::read(const char *uri) {
  // See RFC2396
  // Does not support:
  //   opaque URI data (Section 3)
  //   registry-based naming (Section 3.2.1)
  //   path parameters (Section 3.3)

  clear(); // Reset all

  const char *s = uri;
  try {
    if (!*s) THROW("Cannot be empty");

    if (*s == '/') parsePath(s);
    else {
      parseScheme(s);
      parseNetPath(s);
    }
    if (consume(s, '?')) parseQuery(s);

  } catch (const Exception &e) {
    THROW("Failed to parse URI '" << String::escapeC(uri) << "' at char "
           << (s - uri) << ": " << e.getMessage());
  }

  if (*s) THROW("URI parse incomplete: " << uri);
}


ostream &URI::write(ostream &stream) const {
  if (!scheme.empty()) stream << scheme << ':';
  if (!host.empty()) {
    stream << "//";

    if (!user.empty()) stream << encode(user, USER_PASS_CHARS);
    if (!pass.empty()) stream << ':' << encode(pass, USER_PASS_CHARS);
    if (!user.empty() || !pass.empty()) stream << '@';

    stream << encode(host, HOST_CHARS);
    if (port && port != portFromScheme(scheme)) stream << ':' << port;
  }

  stream << getEscapedPath();

  if (!empty()) stream << '?';

  return writeQuery(stream);
}


ostream &URI::writeQuery(ostream &stream) const {
  for (const_iterator it = begin(); it != end(); it++) {
    if (it != begin()) stream << '&';

    stream << encode(it->first, NAME_CHARS);
    if (!it->second.empty()) stream << '=' << encode(it->second, VALUE_CHARS);
  }

  return stream;
}


string URI::toString() const {
  ostringstream str;
  str << *this;
  return str.str();
}


string URI::encode(const string &s, const char *unescaped) {
  string result;

  for (unsigned i = 0; i < s.length(); i++)
    if (contains(unescaped, s[i])) result.append(1, s[i]);
    else result.append(String::printf("%%%02x", (unsigned)s[i]));

  return result;
}


string URI::decode(const string &s) {
  string result;

  for (unsigned i = 0; i < s.length(); i++)
    if (s[i] == '%' && i < s.length() - 2 &&
        isxdigit(s[i + 1]) && isxdigit(s[i + 2])) {
      result.append(1, nibblesToChar(s[i + 1], s[i + 2]));
      i += 2;

    } else result.append(1, s[i]);

  return result;
}


char URI::parseEscape(const char *&s) {
  match(s, '%');

  char a, b;
  if (!isxdigit(a = *s++) || !isxdigit(b = *s++))
    THROW("Expected hexadecimal digit in escape sequence");

  return nibblesToChar(a, b);
}


void URI::parsePath(const char *&s) {
  bool absolute = consume(s, '/');

  do {
    parsePathSegment(s);
  } while (consume(s, '/'));

  // TODO This fails for encoded / i.e. %2f
  path = (absolute ? "/" : "") + String::join(pathSegs, "/");
}


void URI::parsePathSegment(const char *&s) {
  string seg;

  while (true)
    if (contains(PATH_SEGMENT_CHARS, *s)) seg.append(1, *s++);
    else if (*s == '%') seg.append(1, parseEscape(s));
    else break;

  pathSegs.push_back(seg);
}


void URI::parseScheme(const char *&s) {
  if (!isalpha(*s)) THROW("Expected alpha at start of scheme");

  while (true)
    if (contains(SCHEME_CHARS, *s)) scheme.append(1, *s++);
    else break;

  match(s, ':');
}


void URI::parseNetPath(const char *&s) {
  match(s, '/');
  consume(s, '/');
  parseAuthority(s);
  if (*s == '/') parsePath(s);
}


void URI::parseAuthority(const char *&s) {
  // Try parsing user info
  const char *save = s;
  parseUserInfo(s);
  if (!consume(s, '@')) {
    // Backtrack, wasn't user info
    s = save;
    user.clear();
    pass.clear();
  }

  parseHost(s);
  if (consume(s, ':')) parsePort(s);
}


string URI::parseUserPass(const char *&s) {
  string result;

  while (true)
    if (contains(USER_PASS_CHARS, *s)) result.append(1, *s++);
    else if (*s == '%') result.append(1, parseEscape(s));
    else break;

  return result;
}


void URI::parseUserInfo(const char *&s) {
  user = parseUserPass(s);
  if (consume(s, ':')) pass = parseUserPass(s);
}


void URI::parseHost(const char *&s) {
  while (true)
    if (contains(HOST_CHARS, *s)) host.append(1, *s++);
    else break;

  if (host.empty()) THROW("Expected host character");
}


void URI::parsePort(const char *&s) {
  string result;

  while (isdigit(*s)) result.append(1, *s++);

  if (result.empty()) return;
  port = String::parseU32(result);
}


void URI::parseQuery(const char *&s) {
  if (!*s) return; // Empty query
  parsePair(s);
  while (consume(s, '&')) parsePair(s);
}


void URI::parsePair(const char *&s) {
  string name  = parseName(s);
  string value = consume(s, '=') ? parseValue(s) : "";
  set(name, value);
}


string URI::parseName(const char *&s) {
  string result;

  while (true)
    if (contains(NAME_CHARS, *s)) result.append(1, *s++);
    else if (*s == '%') result.append(1, parseEscape(s));
    else break;

  if (result.empty()) THROW("Expected query name character");

  return result;
}


string URI::parseValue(const char *&s) {
  string result;

  while (true)
    if (contains(VALUE_CHARS, *s)) result.append(1, *s++);
    else if (*s == '%') result.append(1, parseEscape(s));
    else break;

  return result;
}
