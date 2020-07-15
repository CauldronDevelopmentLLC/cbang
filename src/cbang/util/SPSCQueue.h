/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include <atomic>


namespace cb {
  /// A lockless Single Producer Single Consumer queue.
  /// See https://www.drdobbs.com/210604448?pgno=2
  template <typename T>
  class SPSCQueue {
    struct Node {
      T value;
      Node *next = 0;

      Node(T value = T()) : value(value) {}
    };

    std::atomic<Node *> head;
    std::atomic<Node *> tail;
    std::atomic<unsigned> count;

    // Prevent copying
    SPSCQueue(const SPSCQueue &) {}
    SPSCQueue &operator=(const SPSCQueue &) {}

  public:
    SPSCQueue() : count(0) {head = tail = new Node;}
    ~SPSCQueue() {clear(); delete head;}


    bool empty() const {return head == tail;}
    unsigned size() const {return count;}


    void push(const T &value) {
      (*tail).next = new Node(value);
      tail = (*tail).next;
      count++;
    }


    T &top() {
      if (empty()) CBANG_THROW("Queue empty");
      return (*head).next->value;
    }


    void pop() {
      if (empty()) CBANG_THROW("Queue empty");
      Node *n = head;
      head = (*head).next;
      count--;
      (*head).value = T();
      delete n;
    }


    void clear() {while (!empty()) pop();}
  };
}
