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

#include <cbang/Exception.h>

#include <moodycamel/readerwriterqueue.h>


namespace cb {
  template <typename T>
  class SPSCQueue : private moodycamel::ReaderWriterQueue<T> {
    typedef moodycamel::ReaderWriterQueue<T> Super_T;

    // Prevent copying
    SPSCQueue(const SPSCQueue &) {}
    SPSCQueue &operator=(const SPSCQueue &) {}

  public:
    SPSCQueue(unsigned capacity = 1024) : Super_T(capacity) {}

    bool empty() const {return !Super_T::peek();}
    void push(const T &value) {Super_T::enqueue(value);}


    T &top() {
      T *value = Super_T::peek();
      if (!value) CBANG_THROW("Queue empty");
      return *value;
    }


    void pop() {if (!Super_T::pop()) CBANG_THROW("Queue empty");}
  };
}
