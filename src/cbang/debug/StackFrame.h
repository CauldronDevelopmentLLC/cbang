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

#include <cbang/FileLocation.h>

#include <ostream>

namespace cb {
  class StackFrame {
    void *addr;
    const FileLocation *location = 0;

  public:
    StackFrame(void *addr) : addr(addr) {}

    void *getAddr() const {return addr;}
    std::string getAddrString() const;

    void setLocation(const FileLocation *location) {this->location = location;}
    const FileLocation *getLocation() const {return location;}
    const std::string &getFunction() const;

    std::ostream &print(std::ostream &stream) const;
    void write(cb::JSON::Sink &sink) const;
  };

  inline std::ostream &operator<<(std::ostream &stream, const StackFrame &f) {
    return f.print(stream);
  }
}
