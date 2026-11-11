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

// Session ID and OAuth2 state checks.  Prints properties of random values,
// never the values, so the output is deterministic.
//
//   authCheck <SessionID|OAuthState|Rotate>

#include <cbang/Catch.h>
#include <cbang/net/Base64.h>
#include <cbang/net/URI.h>
#include <cbang/net/SockAddr.h>
#include <cbang/http/SessionManager.h>
#include <cbang/oauth2/Provider.h>
#include <cbang/log/Logger.h>
#include <cbang/config/Options.h>

#include <iostream>
#include <set>
#include <map>

using namespace cb;
using namespace std;


namespace {
  class TestProvider : public OAuth2::Provider {
  public:
    TestProvider() : Provider("https://auth.example.com/auth",
                              "https://auth.example.com/token",
                              "https://auth.example.com/profile", "email") {}

    // From OAuth2::Provider
    const char *getName() const override {return "test";}
    JSON::ValuePtr processProfile(const JSON::ValuePtr &p) const override {
      return p;
    }
  };


  void check(const string &name, bool ok) {
    cout << name << ": " << (ok ? "yes" : "NO") << "\n";
  }


  void sessionID() {
    HTTP::SessionManager sm;
    const unsigned n = 10000;
    set<string> ids;
    bool length = true, charset = true, bytes = true;
    unsigned ones = 0;
    map<unsigned, set<char>> perPos;

    for (unsigned i = 0; i < n; i++) {
      string id = sm.generateID(SockAddr());
      ids.insert(id);
      if (id.length() != 43) length = false;
      for (unsigned j = 0; j < id.length(); j++) {
        char c = id[j];
        if (!(isalnum(c) || c == '-' || c == '_')) charset = false;
        perPos[j].insert(c);
      }

      string raw = URLBase64().decode(id);
      if (raw.length() != 32) bytes = false;
      for (unsigned char c: raw) ones += __builtin_popcount(c);
    }

    double frac = (double)ones / (n * 256.0);
    unsigned minPos = 64;
    for (unsigned j = 0; j < 42; j++) // The last char has only 4 bits
      minPos = min(minPos, (unsigned)perPos[j].size());

    check("43 characters", length);
    check("URL Base64 characters only", charset);
    check("256 bits", bytes);
    check("all unique", ids.size() == n);
    check("half the bits set (0.49 to 0.51)", 0.49 < frac && frac < 0.51);
    check("all 64 characters at each position", minPos == 64);
  }


  void oauthState() {
    TestProvider p;
    Options options;
    p.addOptions(options);
    options["test-redirect-base"].set("https://app.example.com");
    options["test-client-id"].set("client");
    options["test-client-secret"].set("secret");

    HTTP::SessionManager sm;
    string sid   = sm.generateID(SockAddr());
    string other = sm.generateID(SockAddr());

    string s1 = p.makeState(sid);
    string s2 = p.makeState(sid);
    size_t dot = s1.find('.');

    check("state is not the session ID", s1 != sid);
    check("state does not contain the session ID",
          s1.find(sid) == string::npos);
    check("nonce of 128 bits",
          dot != string::npos && URLBase64().decode(s1.substr(0, dot))
          .length() == 16);
    check("a new state each time", s1 != s2);
    check("state checks for its session", p.checkState(s1, sid));
    check("state refused for another session", !p.checkState(s1, other));
    check("session ID as state refused", !p.checkState(sid, sid));
    check("empty state refused", !p.checkState("", sid));
    check("empty key refused", !p.checkState(s1, ""));

    string tampered = s1;
    tampered[tampered.length() - 2] ^= 1;
    check("tampered HMAC refused", !p.checkState(tampered, sid));
    string swapped = s2.substr(0, s2.find('.')) + s1.substr(dot);
    check("another nonce refused", !p.checkState(swapped, sid));

    // Through the redirect and callback URLs
    URI redirect = p.getRedirectURL("/login/test", sid);
    string state = redirect.get("state");
    check("redirect state is not the session ID", state != sid);
    check("redirect URL does not contain the session ID",
          redirect.toString().find(sid) == string::npos);

    URI callback("https://app.example.com/login/test");
    callback.set("code", "c");
    callback.set("state", state);
    check("callback accepted", !p.isForgery(callback, sid));
    check("callback for another session refused",
          p.isForgery(callback, other));

    URI noCode("https://app.example.com/login/test");
    noCode.set("state", state);
    check("callback without code refused", p.isForgery(noCode, sid));

    URI legacy("https://app.example.com/login/test");
    legacy.set("code", "c");
    legacy.set("state", sid);
    check("callback with the session ID as state refused",
          p.isForgery(legacy, sid));
  }


  void rotate() {
    HTTP::SessionManager sm;
    auto session = sm.openSession(SockAddr());
    session->setUser("alice");
    string before = session->getID();

    string old = sm.rotateSession(session);
    string after = session->getID();

    check("returns the old ID", old == before);
    check("new ID differs", after != before);
    check("new ID is 43 characters", after.length() == 43);
    check("old ID closed", !sm.hasSession(before));
    check("new ID open", sm.hasSession(after));
    check("same session under the new ID",
          sm.lookupSession(after).get() == session.get());
    check("session data kept", session->getUser() == "alice");
    check("rotate-on-login on by default", sm.getRotateOnLogin());
    check("login-reply-id off by default", !sm.getLoginReplyID());
  }
}


int main(int argc, char *argv[]) {
  try {
    Logger::instance().setScreenStream(cerr);
    if (argc != 2) {
      cerr << "Usage: " << argv[0] << " <SessionID|OAuthState|Rotate>\n";
      return 1;
    }

    string s = argv[1];
    if (s == "SessionID") sessionID();
    else if (s == "OAuthState") oauthState();
    else if (s == "Rotate") rotate();
    else THROW("Unknown scenario: " << s);

    return 0;
  } CATCH_ERROR;

  return 1;
}
