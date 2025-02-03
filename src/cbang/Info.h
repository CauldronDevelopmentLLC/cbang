/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include <cbang/util/Singleton.h>
#include <cbang/util/OrderedDict.h>

#include <string>
#include <ostream>


namespace cb {
  namespace JSON {class Value; class Sink;}
  namespace XML {class Writer;}

  class Info : public Singleton<Info> {
    typedef OrderedDict<std::string, std::string> category_t;
    typedef OrderedDict<std::string, category_t> categories_t;

    unsigned maxKeyLength;
    categories_t categories;

  public:
    Info(Inaccessible) : maxKeyLength(0) {}

    category_t &add(const std::string &category, bool prepend = false);
    void add(const std::string &category, const std::string &key,
             const std::string &value, bool prepend = false);
    const std::string &get(const std::string &category,
                           const std::string &key) const;
    bool has(const std::string &category, const std::string &key) const;


    std::ostream &print(std::ostream &stream, unsigned width = 80,
                        bool wrap = true) const;
    void write(XML::Writer &writer) const;
    void writeList(JSON::Sink &sink) const;
    void write(JSON::Sink &sink) const;
  };

  inline static
  std::ostream &operator<<(std::ostream &stream, const Info &info) {
    return info.print(stream);
  }
}
