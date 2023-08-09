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

#include "IOBuffer.h"

namespace cb {
  class MemoryBuffer : public IOBuffer {
    unsigned capacity;
    unsigned position;
    unsigned fill;
    char *buffer;
    bool deallocate;

  public:
    MemoryBuffer(unsigned capacity = 0, char *buffer = 0,
                 bool deallocate = false);
    ~MemoryBuffer();

    // From IOBuffer
    unsigned getFill() const override {return fill - position;}
    unsigned getSpace() const override {return capacity - fill;}
    unsigned read(char *dst, unsigned size) override;
    unsigned write(const char *src, unsigned size) override;
    unsigned writeTo(std::ostream &stream) override;
    unsigned readFrom(std::istream &stream) override;

    bool isClear() const {return fill == 0 && position == 0;}

    unsigned getPosition() const {return position;}
    unsigned getCapacity() const {return capacity;}

    void incPosition(unsigned count) {position += count;}
    void incFill(unsigned count) {fill += count;}
    void seek(unsigned position) {this->position = position;}
    void shiftToFront();

    const char *begin() const {return buffer + position;}
    char *begin() {return buffer + position;}

    const char *end() const {return buffer + fill;}
    char *end() {return buffer + fill;}

    unsigned increase(unsigned size);
    void clear() {position = fill = 0;}
  };
}
