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

#include "Value.h"

#include <vector>


namespace cb {
  namespace JSON {
    using ListImpl = std::vector<ValuePtr>;

    class List : public Value, protected ListImpl {
      bool simple = true;

    public:
      using ListImpl::ListImpl;

      // From Value
      ValueType getType() const override {return JSON_LIST;}
      bool isList() const override {return true;}
      ValuePtr copy(bool deep = false) const override;
      bool isSimple() const override {return simple;}

      Value &getList() override {return *this;}
      const Value &getList() const override {return *this;}

      Iterator begin() const override;
      Iterator end()   const override;
      Iterator find(unsigned i) const override;

      bool toBoolean() const override {return size();}
      unsigned size() const override {return ListImpl::size();}

      const ValuePtr &get(unsigned i) const override;

      void append(const ValuePtr &value) override;
      void set(unsigned i, const ValuePtr &value) override;
      void clear() override {ListImpl::clear();}
      void erase(unsigned i) override;
      Iterator erase(const Iterator &it) override;

      void write(Sink &sink) const override;

      void visitChildren(
        const_visitor_t visitor, bool depthFirst = true) const override;
      void visitChildren(visitor_t visitor, bool depthFirst = true) override;

      using Value::getList;
      using Value::get;
      using Value::append;
      using Value::set;
      using Value::insert;
      using Value::find;
      using Value::erase;
      using Value::empty;

    protected:
      void check(unsigned i) const;
      Iterator makeIt(const ListImpl::const_iterator &it) const;
    };
  }
}
