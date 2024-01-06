/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

namespace cb {
  template <typename T, unsigned SIZE>
  class Array {
    typedef Array<T, SIZE> Type_T;

    T array[SIZE];

  public:
    Array(const T init = T()) {
      if (!SIZE) CBANG_THROW("Cannot allocate zero length array");
      for (unsigned i = 0; i < SIZE; i++) array[i] = init;
    }

    Array(const T array[SIZE]) {
      if (!SIZE) CBANG_THROW("Cannot allocate zero length array");
      for (unsigned i = 0; i < SIZE; i++) this->array[i] = array[i];
    }

    Array(const Type_T &o) {
      if (!SIZE) CBANG_THROW("Cannot allocate zero length array");
      for (unsigned i = 0; i < SIZE; i++) array[i] = o.array[i];
    }

    const unsigned size() const {return SIZE;}

    const T *data() const {return array;}
    T *data() {return array;}

    T &get(unsigned i) {
      if (SIZE <= i) CBANG_THROW("Array index out of range");
      return array[i];
    }

    const T &get(unsigned i) const {
      if (SIZE <= i) CBANG_THROW("Array index out of range");
      return array[i];
    }

    Type_T &operator=(const Type_T &o) {
      for (unsigned i = 0; i < SIZE; i++) array[i] = o.array[i];
      return *this;
    }

    T &operator[](unsigned i) {return get(i);}
    const T &operator[](unsigned i) const {return get(i);}

    T &operator*() {return get(0);}
    const T &operator*() const {return get(0);}

    operator T *() {return array;}
    operator const T *() const {return array;}

    bool operator==(const Type_T &o) const {
      for (unsigned i = 0; i < SIZE; i++)
        if (array[i] != o.array[i]) return false;
      return true;
    }
    bool operator!=(const Type_T &o) const {return !(*this == o);}
  };
}
