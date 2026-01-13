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

#include "StringICmp.h"

#include <cbang/Exception.h>

#include <map>
#include <iostream>


namespace cb {
  template <typename LESS_T = std::less<std::string> >
  class StringMapBase : public std::map<std::string, std::string, LESS_T> {
  public:
    typedef std::map<std::string, std::string, LESS_T> Super_T;
    typedef typename Super_T::iterator iterator;
    typedef typename Super_T::const_iterator const_iterator;
    typedef typename Super_T::value_type value_type;
    using Super_T::find;
    using Super_T::end;
    using Super_T::insert;


    void set(const std::string &key, const std::string &value) {
      auto result = insert(value_type(key, value));
      if (!result.second) result.first->second = value;
    }


    void unset(const std::string &key) {Super_T::erase(key);}


    void append(const std::string &key, const std::string &value) {
      auto it = find(key);
      if (it == end()) set(key, value);
      else it->second += value;
    }


    const std::string &get(const std::string &key) const {
      auto it = find(key);
      if (it == end()) CBANG_THROW("'" << key << "' not set");
      return it->second;
    }


    const std::string &get(const std::string &key,
                           const std::string &defaultValue) const {
      auto it = Super_T::find(key);
      return it == end() ? defaultValue : it->second;
    }


    using Super_T::operator[];
    const std::string &operator[](const std::string &key) const
    {return get(key);}


    bool has(const std::string &key) const {return find(key) != end();}
  };


  typedef StringMapBase<> StringMap;
  typedef StringMapBase<StringILess> CIStringMap;
}
