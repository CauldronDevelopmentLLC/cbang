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

#include <cbang/SmartPointer.h>

#include <map>
#include <string>


namespace cb {
  template <typename Key, typename Value, typename KeyLess = std::less<Key>>
  class OrderedDict {
  private:
    struct MapEntry;

    using dict_t = std::map<Key, SmartPointer<struct MapEntry>, KeyLess>;

    struct MapEntry {
      Value value;
      MapEntry *prev = 0;
      MapEntry *next = 0;
      typename dict_t::iterator it;

      MapEntry(const Value &value) : value(value) {}
    };

    struct Iterator;

    struct ConstIterator {
      MapEntry *e;

      ConstIterator(MapEntry *e = 0) : e(e) {}
      ConstIterator(const ConstIterator &o) : e(o.e) {}
      ConstIterator(const Iterator &o);

      ConstIterator &operator=(const ConstIterator &o) {e = o.e; return *this;}
      bool operator==(const ConstIterator &o) const {return e == o.e;}
      bool operator!=(const ConstIterator &o) const {return e != o.e;}


      ConstIterator &operator++() {
        if (e) e = e->next;
        return *this;
      }


      ConstIterator operator++(int) {
        auto saveE = e;
        if (e) e = e->next;
        return ConstIterator(saveE);
      }


      ConstIterator &operator--() {
        if (e) e = e->prev;
        return *this;
      }


      ConstIterator operator--(int) {
        auto saveE = e;
        if (e) e = e->prev;
        return ConstIterator(saveE);
      }

      const Key   &key()        const {return  deref().it->first;}
      const Value &value()      const {return  deref().value;}
      const Value *operator->() const {return &value();}
      const Value &operator*()  const {return  value();}


      MapEntry &deref() const {
        if (!e) CBANG_THROW("Cannot dereference null iterator");
        return *e;
      }
    };


    struct Iterator : public ConstIterator {
      using ConstIterator::ConstIterator;
      using ConstIterator::deref;

      Value &value()      const {return  deref().value;}
      Value *operator->() const {return &value();}
      Value &operator*()  const {return  value();}
    };


    struct EntriesIterator : public Iterator {
      using Iterator::Iterator;
      const Iterator &operator*()  const {return *this;}
    };


    struct ConstEntriesIterator : public ConstIterator {
      using ConstIterator::ConstIterator;
      const ConstIterator &operator*()  const {return *this;}
    };


    dict_t dict;
    MapEntry *head = 0;
    MapEntry *tail = 0;

  public:
    using size_type = typename dict_t::size_type;


    OrderedDict() {}
    OrderedDict(const OrderedDict &o) {*this = o;}
    OrderedDict(OrderedDict &&o) = default;

    OrderedDict &operator=(OrderedDict &&o) = default;


    OrderedDict &operator=(const OrderedDict &o) {
      if (this != &o) {
        clear();
        for (auto it: o) insert(it.key(), it.value());
      }

      return *this;
    }


    void clear() {dict.clear(); head = tail = 0;}
    bool empty() const {return dict.empty();}
    size_type size() const {return dict.size();}

    using iterator       = EntriesIterator;
    using const_iterator = ConstEntriesIterator;
    const_iterator begin() const {return const_iterator(head);}
    const_iterator end()   const {return const_iterator(0);}
    iterator       begin()       {return iterator(head);}
    iterator       end()         {return iterator(0);}


    const_iterator find(const Key &key) const {
      auto it = dict.find(key);
      return it == dict.end() ? end() : const_iterator(it->second.get());
    }


    iterator find(const Key &key) {
      auto it = dict.find(key);
      return it == dict.end() ? end() : iterator(it->second.get());
    }


    iterator insert(const Key &key, const Value &value, bool prepend = false) {
      auto it = dict.find(key);
      if (it == dict.end()) it = _insert(key, value, prepend);
      else it->second->value = value;
      return iterator(it->second.get());
    }


    Value &operator[](const Key &key) {
      auto it = dict.find(key);
      if (it == dict.end()) it = _insert(key);
      return it->second->value;
    }


    iterator erase(const Key &key) {
      auto it = dict.find(key);
      if (it == dict.end()) return end();
      return erase(iterator(it->second.get()));
    }


    iterator erase(iterator it) {
      auto e = it.e;
      if (!e) CBANG_THROW("Cannot erase empty iterator");

      auto next = e->next;
      _erase(e);
      return iterator(next);
    }


  private:
    const typename dict_t::iterator &_insert(
      const Key &key, const Value &value = Value(), bool prepend = false) {
      auto e = new MapEntry(value);

      if (prepend) {
        e->next = head;
        if (head) head->prev = e;
        if (!tail) tail = e;
        head = e;

      } else {
        e->prev = tail;
        if (tail) tail->next = e;
        if (!head) head = e;
        tail = e;
      }

      return e->it = dict.insert(typename dict_t::value_type(key, e)).first;
    }


    void _erase(MapEntry *e) {
      if (head == e) head = e->next;
      if (tail == e) tail = e->prev;
      if (e->prev) e->prev->next = e->next;
      if (e->next) e->next->prev = e->prev;

      dict.erase(e->it);
    }
  };

  template <typename Key, typename Value, typename KeyLess>
  OrderedDict<Key, Value, KeyLess>::ConstIterator::ConstIterator(
    const Iterator &o) : e(o.e) {}
}
