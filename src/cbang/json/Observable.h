/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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
      Value *parent = 0;
      ValuePtr key;

    public:
      void clearParentRef() {parent = 0;}
      void setParentRef(Value *parent, const ValuePtr &key);
      void incParentRef();
      void decParentRef();

      virtual void notify(const std::list<ValuePtr> &change) {}

      void _notify(std::list<ValuePtr> &change);
      void _notify(unsigned index, const ValuePtr &value = 0);
      void _notify(const std::string &key, const ValuePtr &value = 0);
    };


    template <typename T>
    class Observable : public ObservableBase, public T {
    public:
      ~Observable() {
        for (auto &v: *this)
          _clearParentRef(v);
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
        auto i = T::size();
        T::append(value);
        _setParentRef(value, i);
        _notify(i, value);
      }


      void set(unsigned i, const ValuePtr &_value) override {
        // Block operation if value is equal
        auto value = T::get(i);
        if (*value == *_value) return;

        _clearParentRef(value);

        value = convert(_value);
        T::set(i, value);
        _setParentRef(value, i);
        _notify(i, value);
      }


      Iterator insert(const std::string &key, const ValuePtr &_value) override {
        // Block operation if value is equal
        auto it = T::find(key);

        if (it) {
          if (*_value == **it) return it;
          _clearParentRef(*it);
        }

        auto value = convert(_value);
        it = T::insert(key, value);
        _setParentRef(value, key);
        _notify(key, value);

        return it;
      }


      void clear() override {
        for (auto &v: *this)
          _clearParentRef(v);

        T::clear();

        std::list<ValuePtr> change;
        change.push_front(T::isList() ? T::createList() : T::createDict());
        _notify(change);
      }


      Iterator erase(const Iterator &it) override {
        _clearParentRef(*it);
        auto it2 = T::erase(it);

        if (T::isList()) {
          while (it2) _decParentRef(*it2++);
          _notify(it.index());

        } else _notify(it.key());

        return it2;
      }


      void erase(unsigned i) override {
        _clearParentRef(T::get(i));
        T::erase(i);

        if (T::isList())
          for (unsigned j = i; j < T::size(); j++)
            _decParentRef(T::get(j));

        _notify(i);
      }


      void erase(const std::string &key) override {
        auto it = T::find(key);
        if (!it) return;
        _clearParentRef(*it);
        it = T::erase(it);
        while (it) _decParentRef(*it++);
        _notify(key);
      }


    private:
      static void _clearParentRef(const ValuePtr &target) {
        auto *o = dynamic_cast<ObservableBase *>(target.get());
        if (o) o->clearParentRef();
      }


      void _setParentRef(const ValuePtr &target, const ValuePtr &key) {
        auto *o = dynamic_cast<ObservableBase *>(target.get());
        if (o) o->setParentRef(this, key);
      }


      void _setParentRef(const ValuePtr &target, unsigned i) {
        _setParentRef(target, T::create(i));
      }


      void _setParentRef(const ValuePtr &target, const std::string &key) {
        _setParentRef(target, T::create(key));
      }


      static void _incParentRef(const ValuePtr &target) {
        auto *o = dynamic_cast<ObservableBase *>(target.get());
        if (o) o->incParentRef();
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
