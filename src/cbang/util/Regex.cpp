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

#include "Regex.h"

#include <re2/re2.h>

using namespace cb;
using namespace std;


struct Regex::private_t {
  RE2 re;

  private_t(const string &pattern, const RE2::Options &opts) :
    re(pattern, opts) {
    if (!re.ok()) THROW("Failed to parse Regex: " << re.error());
  }
};


Regex::Regex(const string &pattern, bool posix) {
  RE2::Options opts;

  opts.set_log_errors(false);

  if (posix) {
    opts.set_posix_syntax(true);
    opts.set_perl_classes(true);
    opts.set_word_boundary(true);
  }

  pri = new private_t(pattern, opts);
}


string Regex::toString() const {return pri->re.pattern();}


unsigned Regex::getGroupCount() const {
  int n = pri->re.NumberOfCapturingGroups();
  return n < 0 ? 0 : (unsigned)n;
}


const map<string, int> &Regex::getGroupNameMap() const {
  return pri->re.NamedCapturingGroups();
}


const map<int, string> &Regex::getGroupIndexMap() const {
  return pri->re.CapturingGroupNames();
}


bool Regex::match(const string &s) const {
  return RE2::FullMatchN(s, pri->re, 0, 0);
}


bool Regex::match(const string &s, Match &m) const {
  return match_or_search(true, s, m);
}


bool Regex::search(const string &s) const {
  return RE2::PartialMatchN(s, pri->re, 0, 0);
}


bool Regex::search(const string &s, Match &m) const {
  return match_or_search(false, s, m);
}


string Regex::replace(const string &s, const string &r) const {
  string result = s;
  RE2::Replace(&result, pri->re, r);
  return result;
}


string Regex::escape(const string &s) {return RE2::QuoteMeta(s);}


bool Regex::match_or_search(bool match, const string &s, Match &m) const {
  unsigned n = getGroupCount();

  vector<RE2::Arg>         args(n);
  vector<RE2::Arg *>       argPtrs(n);
  vector<re2::StringPiece> groups(n);

  // Connect args
  for (unsigned i = 0; i < n; i++) {
    args[i]    = &groups[i];
    argPtrs[i] = &args[i];
  }

  auto fn = match ? &RE2::FullMatchN : &RE2::PartialMatchN;
  if ((*fn)(s, pri->re, argPtrs.data(), n)) {
    m.clear();
    m.offsets.clear();

    for (unsigned i = 0; i < n; i++) {
      m.push_back(string(groups[i].data(), groups[i].size()));
      m.offsets.push_back(groups[i].data() - s.data());
    }

    return true;
  }

  return false;
}
