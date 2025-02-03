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

#include "Factory.h"

#include <functional>


namespace cb {
  namespace JSON {
    class Path {
      typedef std::vector<std::string> parts_t;
      parts_t parts;

    public:
      Path(parts_t::const_iterator begin, parts_t::const_iterator end) :
        parts(begin, end) {}
      Path(const parts_t &parts) : parts(parts.begin(), parts.end()) {}
      Path(const std::string &path);

      bool empty() const {return parts.empty();}
      unsigned size() const {return parts.size();}
      std::string toString(unsigned start = 0, int end = -1) const;

      std::string pop();
      void push(const std::string &part);

      typedef std::function <ValuePtr (unsigned i)> fail_cb_t;

      ValuePtr select(const Value &value, fail_cb_t fail_cb = 0) const;
      ValuePtr select(const Value &value, const ValuePtr &defaultValue) const;
      bool     exists(const Value &value) const;
      void     modify(Value &target, const ValuePtr &value);
      void     insert(Value &target, const ValuePtr &value)
        {modify(target, value);}
      void     erase(Value &target) {modify(target, 0);}

#define CBANG_JSON_VT(NAME, TYPE, ...)                                  \
      TYPE select##NAME(const Value &value) const;                      \
      TYPE select##NAME(const Value &value, TYPE defaultValue) const;   \
      bool exists##NAME(const Value &value) const;
#include "ValueTypes.def"
   };
  }
}
