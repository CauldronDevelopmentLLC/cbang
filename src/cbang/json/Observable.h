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

#include "Dict.h"
#include "List.h"

#include <functional>


namespace cb {
  namespace JSON {
    class ObservableBase {
      Value *parent  = 0;
      unsigned index = 0;

    public:
      void clearParentRef() {parent = 0;}
      void setParentRef(Value *parent, unsigned index);
      void decParentRef() {index--;}

      virtual void notify(const std::list<ValuePtr> &change) {}

      void _notify(std::list<ValuePtr> &change);
      void _notify(unsigned index, const ValuePtr &value = 0);
      void _notify(const std::string &key, const ValuePtr &value = 0);
    };


    template <typename T>
    class Observable : public ObservableBase, public T {
    public:
      ~Observable() {
        for (unsigned i = 0; i < T::size(); i++)
          _clearParentRef(T::get(i));
      }


      ValuePtr convert(const ValuePtr &value) {
        if (value.isInstance<Dict>() && !value.isInstance<ObservableBase>()) {
          ValuePtr d = createDict();
          d->merge(*value);
          return d;
        }

        if (value.isInstance<List>() && !value.isInstance<ObservableBase>()) {
          ValuePtr l = createList();
          l->appendFrom(*value);
          return l;
        }

        return value;
      }


      // From Value
      void append(const ValuePtr &_value) override {
        ValuePtr value = convert(_value);
        int i = T::size();
        T::append(value);
        _setParentRef(value, i);
        _notify(i, value);
      }


      void set(unsigned i, const ValuePtr &_value) override {
        // Block operation if value is equal
        ValuePtr current = T::get(i);
        if (*_value == *current) return;

        _clearParentRef(current);

        ValuePtr value = convert(_value);
        T::set(i, value);
        _setParentRef(value, i);
        _notify(i, value);
      }


      int insert(const std::string &key, const ValuePtr &_value) override {
        // Block operation if value is equal
        int index = T::indexOf(key);

        if (index != -1) {
          const ValuePtr &current = T::get(index);
          if (*_value == *current) return index;

          _clearParentRef(current);
        }

        ValuePtr value = convert(_value);
        int i = T::insert(key, value);
        if (i != -1) {
          _setParentRef(value, i);
          _notify(key, value);
        }

        return i;
      }


      void clear() override {
        for (unsigned i = 0; i < T::size(); i++)
          _clearParentRef(T::get(i));

        T::clear();

        std::list<ValuePtr> change;
        change.push_front(T::isList() ? T::createList() : T::createDict());
        _notify(change);
      }


      void erase(unsigned i) override {
        _clearParentRef(T::get(i));
        T::erase(i);
        for (unsigned j = i; j < T::size(); j++)
          _decParentRef(T::get(j));
        _notify(i);
      }


      void erase(const std::string &key) override {
        _clearParentRef(T::get(key));
        T::erase(key);
        _notify(key);
      }


    private:
      static void _clearParentRef(const ValuePtr &target) {
        auto *o = dynamic_cast<ObservableBase *>(target.get());
        if (o) o->clearParentRef();
      }


      void _setParentRef(const ValuePtr &target, unsigned index) {
        auto *o = dynamic_cast<ObservableBase *>(target.get());
        if (o) o->setParentRef(this, index);
      }


      static void _decParentRef(const ValuePtr &target) {
        auto *o = dynamic_cast<ObservableBase *>(target.get());
        if (o) o->decParentRef();
      }


    public:
      using Value::append;
      using Value::set;
      using Value::insert;
      using Value::begin;
      using Value::end;

      // From Factory
      ValuePtr createDict() const override {return new Observable<Dict>;}
      ValuePtr createList() const override {return new Observable<List>;}
    };


    typedef Observable<Dict> ObservableDict;
    typedef Observable<List> ObservableList;
  }
}
