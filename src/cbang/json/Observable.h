/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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
    template <typename T>
    class Observable : public T {
    protected:
      Value *parent = 0;
      unsigned index = 0;

    public:
      ~Observable() {
        for (unsigned i = 0; i < T::size(); i++)
          T::get(i)->clearParentRef();
      }


      ValuePtr convert(const ValuePtr &value) {
        if (value.isInstance<Dict>() && !value.isInstance<Observable<Dict>>()) {
          ValuePtr d = createDict();
          d->merge(*value);
          return d;
        }

        if (value.isInstance<List>() && !value.isInstance<Observable<List>>()) {
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
        value->setParentRef(this, i);
        notify(i, value);
      }


      void set(unsigned i, const ValuePtr &_value) override {
        // Block operation if value is equal
        ValuePtr current = T::get(i);
        if (*_value == *current) return;

        current->clearParentRef();

        ValuePtr value = convert(_value);
        T::set(i, value);
        value->setParentRef(this, i);
        notify(i, value);
      }


      unsigned insert(const std::string &key, const ValuePtr &_value) override {
        // Block operation if value is equal
        int index = T::indexOf(key);

        if (index != -1) {
          const ValuePtr &current = T::get(index);
          if (*_value == *current) return index;

          current->clearParentRef();
        }

        ValuePtr value = convert(_value);
        unsigned i = T::insert(key, value);
        value->setParentRef(this, i);
        notify(key, value);
        return i;
      }


      void clear() override {
        for (unsigned i = 0; i < T::size(); i++)
          T::get(i)->clearParentRef();

        T::clear();

        std::list<ValuePtr> change;
        change.push_front(T::isList() ? T::createList() : T::createDict());
        notify(change);
      }


      void erase(unsigned i) override {
        T::get(i)->clearParentRef();
        T::erase(i);
        for (unsigned j = i; j < T::size(); j++)
          T::get(j)->decParentRef();
        notify(i);
      }


      void erase(const std::string &key) override {
        T::get(key)->clearParentRef();
        T::erase(key);
        notify(key);
      }


      void setParentRef(Value *parent, unsigned index) override {
        if (parent && this->parent) CBANG_THROW("Parent already set");
        this->parent = parent;
        this->index = index;
      }


      void decParentRef() override {index--;}


      void notify(std::list<ValuePtr> &change) override {
        if (!parent) return;

        if (parent->isList()) change.push_front(T::create(index));
        else change.push_front(T::create(parent->keyAt(index)));

        parent->notify(change);
      }


      void notify(unsigned index, const ValuePtr &value = 0) {
        std::list<ValuePtr> change;

        if (value.isSet()) change.push_back(value);
        else change.push_back(T::createNull());

        change.push_front(T::create(index));
        notify(change);
      }


      void notify(const std::string &key, const ValuePtr &value = 0) {
        std::list<ValuePtr> change;

        if (value.isSet()) change.push_back(value);
        else change.push_back(T::createNull());

        change.push_front(T::create(key));
        notify(change);
      }


      using Value::append;
      using Value::set;
      using Value::insert;

      // From Factory
      ValuePtr createDict() const override {return new Observable<Dict>;}
      ValuePtr createList() const override {return new Observable<List>;}
    };


    typedef Observable<Dict> ObservableDict;
    typedef Observable<List> ObservableList;
  }
}
