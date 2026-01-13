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

#include "Value.h"

#include <cbang/util/OrderedDict.h>


namespace cb {
  namespace JSON {
    using DictImpl = OrderedDict<std::string, ValuePtr>;

    class Dict : public Value, protected DictImpl {
      friend class DictIterator;
      bool simple;

    public:
      Dict() : simple(true) {}

      using Iterator = JSON::Iterator;

      // From DictImpl
      using DictImpl::empty;

      // From Value
      using Value::begin;
      using Value::end;
      using Value::get;
      using Value::erase;
      using Value::find;

      ValueType getType() const override {return JSON_DICT;}
      bool isDict() const override {return true;}
      ValuePtr copy(bool deep = false) const override;
      bool isSimple() const override {return simple;}

      using Value::getDict;
      Dict &getDict() override {return *this;}
      const Dict &getDict() const override {return *this;}

      Iterator begin() const override;
      Iterator end()   const override;

      bool toBoolean() const override {return size();}
      unsigned size() const override {return DictImpl::size();}
      Iterator find(const std::string &key) const override;
      const ValuePtr &get(const std::string &key) const override;

      Iterator insert(const std::string &key, const ValuePtr &value) override;
      using Value::insert;

      void clear() override {DictImpl::clear();}
      void erase(const std::string &key) override {DictImpl::erase(key);}
      Iterator erase(const Iterator &it) override;

      void write(Sink &sink) const override;

      void visitChildren(
        const_visitor_t visitor, bool depthFirst = true) const override;
      void visitChildren(visitor_t visitor, bool depthFirst = true) override;

    private:
      Iterator makeIt(const DictImpl::const_iterator &it) const;
    };
  }
}
