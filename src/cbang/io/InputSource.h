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

#include <cbang/SmartPointer.h>
#include <cbang/util/Named.h>

#include <string>
#include <iostream>


namespace cb {
  class Resource;

  class InputSource : public Named {
    cb::SmartPointer<std::istream> stream;

  public:
    InputSource() : Named("<null>") {}
    InputSource(const InputSource &o) : Named(o.getName()), stream(o.stream) {}
    InputSource(const char *array, std::streamsize length = -1,
                const std::string &name = "<memory>");
    InputSource(const std::string &s, const std::string &name = "<memory>");
    InputSource(std::istream &stream, const std::string &name = std::string());
    InputSource(const SmartPointer<std::istream> &stream,
                const std::string &name = std::string());
    InputSource(const Resource &resource);

    static InputSource open(const std::string &filename);

    operator std::istream &() const {return *stream;}
    std::string toString() const;
    std::string getLine(unsigned maxLength = 4096) const;
  };
}
