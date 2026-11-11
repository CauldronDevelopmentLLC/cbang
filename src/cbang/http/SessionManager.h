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

#include "Session.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <map>


namespace cb {
  class Options;

  namespace HTTP {
    class Request;

    class SessionManager : public JSON::Serializable {
      typedef std::map<std::string, SmartPointer<Session> > sessions_t;
      sessions_t sessions;

      uint64_t lifetime    = Time::SEC_PER_DAY;
      uint64_t timeout     = Time::SEC_PER_HOUR;
      std::string cookie   = "sid";
      uint64_t lastCleanup = 0;

      // Session cookie attributes, see sendCookie()
      bool cookieHTTPOnly        = false;
      bool cookieSecure          = true;
      std::string cookieSameSite = "None";

      bool rotateOnLogin = true;  // See rotateSession()
      bool loginReplyID  = false; // See API::Login

    public:
      SessionManager() {}
      SessionManager(Options &options) {addOptions(options);}

      void addOptions(Options &options);

      uint64_t getLifetime() const {return lifetime;}
      void setLifetime(uint64_t lifetime) {this->lifetime = lifetime;}

      uint64_t getTimeout() const {return timeout;}
      void setTimeout(uint64_t timeout) {this->timeout = timeout;}

      const std::string &getSessionCookie() const {return cookie;}
      void setSessionCookie(const std::string &cookie) {this->cookie = cookie;}

      /// Cookie attributes for sendCookie().  The defaults, not HttpOnly,
      /// Secure and SameSite=None, are those the session cookie always had.
      bool getCookieHTTPOnly() const {return cookieHTTPOnly;}
      void setCookieHTTPOnly(bool x) {cookieHTTPOnly = x;}
      bool getCookieSecure() const {return cookieSecure;}
      void setCookieSecure(bool x) {cookieSecure = x;}
      const std::string &getCookieSameSite() const {return cookieSameSite;}
      void setCookieSameSite(const std::string &x) {cookieSameSite = x;}

      /// Whether a login gives the session a new ID, see rotateSession().
      /// Default true.
      bool getRotateOnLogin() const {return rotateOnLogin;}
      void setRotateOnLogin(bool x) {rotateOnLogin = x;}

      /// Whether the reply that starts an OAuth2 login includes the session
      /// ID, as {id, redirect}.  Default false: the ID is in the cookie.
      bool getLoginReplyID() const {return loginReplyID;}
      void setLoginReplyID(bool x) {loginReplyID = x;}

      /// A new session ID: 256 random bits from a CSPRNG, URL Base64
      /// encoded, 43 characters.  The address is not used.
      virtual std::string generateID(const SockAddr &addr);

      /// Sets the session cookie to sid on the response, with the cookie
      /// attributes above.
      virtual void sendCookie(Request &req, const std::string &sid) const;

      /// Gives the session a new ID, so an ID known before a login, e.g.
      /// one an attacker planted in the cookie, is worth nothing after it.
      /// The old ID is closed and the session is kept under the new one.
      /// Returns the old ID.
      virtual std::string rotateSession(const SmartPointer<Session> &session);

      virtual bool isExpired(const Session &session) const;
      virtual bool hasSession(const std::string &sid) const;
      virtual SmartPointer<Session> lookupSession(const std::string &sid) const;
      virtual SmartPointer<Session> openSession(const SockAddr &addr);
      virtual void closeSession(const std::string &sid);
      virtual void addSession(const SmartPointer<Session> &session);
      virtual void cleanup();

      typedef sessions_t::const_iterator iterator;
      iterator begin() const {return sessions.begin();}
      iterator end() const {return sessions.end();}

      // From JSON::Serializable
      void read(const JSON::Value &value) override;
      void write(JSON::Sink &sink) const override;
    };
  }
}
