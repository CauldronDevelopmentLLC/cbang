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

#include "IteratorImpl.h"


namespace cb {
  namespace JSON {
    class Iterator {
      SmartPointer<IteratorImpl> impl;

    public:
      Iterator() {}
      Iterator(const SmartPointer<IteratorImpl> &impl) : impl(impl) {}
      Iterator(const Iterator &o)  : impl(o.impl->clone()) {}
      Iterator(const Iterator &&o) : impl(o.impl) {}

      Iterator &operator=(const Iterator &o);
      Iterator &operator=(const Iterator &&o) {impl = o.impl; return *this;}

      bool operator==(const Iterator &o) const;
      bool operator!=(const Iterator &o) const {return !(*this == o);}

      operator bool() const {return impl.isSet() && impl->operator bool();}

      Iterator &operator++();
      Iterator operator++(int);
      Iterator &operator--();
      Iterator operator--(int);

      const std::string &key() const {return impl->key();}
      const unsigned index() const {return impl->index();}
      const SmartPointer<Value> &value() const {return impl->value();}

      const SmartPointer<Value> &operator*() const {return value();}
    };
  }
}
