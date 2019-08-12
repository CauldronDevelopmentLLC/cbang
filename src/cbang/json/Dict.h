/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "Value.h"

#include <cbang/util/OrderedDict.h>


namespace cb {
  namespace JSON {
    class Dict : public Value, protected OrderedDict<ValuePtr> {
      typedef OrderedDict<ValuePtr> Super_T;

      bool simple;

    public:
      Dict() : simple(true) {}

      // From OrderedDict<ValuePtr>
      using Super_T::empty;
      using Super_T::has;

      // From Value
      ValueType getType() const {return JSON_DICT;}
      bool isDict() const {return true;}
      ValuePtr copy(bool deep = false) const;
      bool isSimple() const {return simple;}

      using Value::getDict;
      Dict &getDict() {return *this;}
      const Dict &getDict() const {return *this;}

      bool toBoolean() const {return size();}
      unsigned size() const {return Super_T::size();}
      const std::string &keyAt(unsigned i) const
      {return Super_T::keyAt(i);}
      int indexOf(const std::string &key) const {return lookup(key);}
      const ValuePtr &get(unsigned i) const
      {return Super_T::get(i);}
      const ValuePtr &get(const std::string &key) const
      {return Super_T::get(key);}

      unsigned insert(const std::string &key, const ValuePtr &value);
      using Value::insert;

      void clear() {Super_T::clear();}
      void erase(unsigned i) {Super_T::erase(i);}
      void erase(const std::string &key) {Super_T::erase(key);}

      void setParent(Value *parent, unsigned index)
        {CBANG_TYPE_ERROR("Not an ObservableDict");}
      void write(Sink &sink) const;

      void visitChildren(const_visitor_t visitor, bool depthFirst = true) const;
      void visitChildren(visitor_t visitor, bool depthFirst = true);
    };
  }
}
