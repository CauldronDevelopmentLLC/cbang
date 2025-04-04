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

#include <cbang/Exception.h>
#include <cbang/debug/Demangle.h>

#include <vector>


namespace cb {
  class Inaccessible {
    Inaccessible() {}
    template <typename T> friend class Singleton;
  };


  class SingletonBase {
  public:
    virtual ~SingletonBase() {}
  };


  class SingletonDealloc {
    static SingletonDealloc *singleton;
    typedef std::vector<SingletonBase *> singletons_t;
    singletons_t singletons;

    SingletonDealloc() {}
  public:

    static SingletonDealloc &instance();

    void add(SingletonBase *singleton) {singletons.push_back(singleton);}
    void deallocate();
  };


  /***
   * Example usage:
   *
   * class A : public Singleton<A> {
   *   A(Inaccessible);
   *   . . .
   *
   * Then in the implementation file:
   *
   *   A *Singleton<A>::singleton = 0;
   *
   *   A::A(Inaccessible) {
   *     . . .
   */
  template <typename T>
  class Singleton : public SingletonBase {
  protected:
    static Singleton<T> *singleton;

    // Restrict access to constructor and destructor
    Singleton();
    virtual ~Singleton();

  public:
    static T &instance();
  };


  template <typename T> Singleton<T> *Singleton<T>::singleton = 0;


  template <typename T> Singleton<T>::Singleton() {
    if (singleton)
      CBANG_THROW("There can be only one. . .instance of singleton "
                  << type_name<T>());

    singleton = this;
    SingletonDealloc::instance().add(singleton);
  }


  template <typename T> Singleton<T>::~Singleton() {singleton = 0;}


  template <typename T> T &Singleton<T>::instance() {
    if (!singleton) new T(Inaccessible());

    T *ptr = dynamic_cast<T *>(singleton);
    if (!ptr) CBANG_THROW("Invalid singleton, not of type "
                          << type_name<T>());

    return *ptr;
  }
}
