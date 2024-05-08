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

#include <cbang/util/StringMap.h>

#include <string>
#include <vector>


namespace cb {
  class URI : public StringMap {
  protected:
    std::string scheme;
    std::string host;
    unsigned port = 0;
    std::string path;
    std::vector<std::string> pathSegs;
    std::string user;
    std::string pass;

  public:
    static const char *DEFAULT_UNESCAPED;

    URI() {}
    URI(const std::string &uri) {read(uri);}
    URI(const char *uri) {read(uri);}
    URI(const std::string &scheme, const std::string &host, unsigned port = 0,
        const std::string &path = "/");

    const std::string &getScheme() const {return scheme;}
    std::string getAddress() const;
    const std::string &getHost() const {return host;}
    unsigned getPort() const;
    const std::string &getPath() const {return path;}
    std::string getEscapedPath() const;
    const std::vector<std::string> &getPathSegments() const {return pathSegs;}
    std::string getExtension() const;
    const std::string &getUser() const {return user;}
    const std::string &getPass() const {return pass;}
    const std::string getQuery() const;

    void setScheme(const std::string &scheme) {this->scheme = scheme;}
    void setHost(const std::string &host) {this->host = host;}
    void setPort(unsigned port) {this->port = port;}
    void setPath(const std::string &path);
    void setUser(const std::string &user) {this->user = user;}
    void setPass(const std::string &pass) {this->pass = pass;}
    void setQuery(const std::string &query);
    void setQuery(const char *query);

    static unsigned portFromScheme(const std::string &scheme);
    static bool schemeRequiresSSL(const std::string &scheme);
    bool schemeRequiresSSL() const;

    void clear();
    void normalize();

    void read(const std::string &s);
    void read(const char *s);
    std::ostream &write(std::ostream &stream) const;
    std::ostream &writeQuery(std::ostream &stream) const;

    std::string toString() const;
    operator std::string() const {return toString();}

    static std::string encode(const std::string &s,
                              const char *unescaped = DEFAULT_UNESCAPED);
    static std::string decode(const std::string &s);

  protected:
    char parseEscape(const char *&s);
    void parsePath(const char *&s);
    void parsePathSegment(const char *&s);
    void parseScheme(const char *&s);
    void parseNetPath(const char *&s);
    void parseAuthority(const char *&s);
    std::string parseUserPass(const char *&s);
    void parseUserInfo(const char *&s);
    void parseHost(const char *&s);
    void parsePort(const char *&s);
    void parseQuery(const char *&s);
    void parsePair(const char *&s);
    std::string parseName(const char *&s);
    std::string parseValue(const char *&s);
  };


  inline std::ostream &operator<<(std::ostream &stream, const URI &uri) {
    return uri.write(stream);
  }
}
