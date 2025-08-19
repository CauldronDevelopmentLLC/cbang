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

#include "Option.h"

#include <cbang/SmartPointer.h>

#include <map>
#include <string>

namespace cb {
  namespace XML {class Handler;}
  namespace JSON {class Sink;}

  class OptionCategory {
    typedef std::map<const std::string, SmartPointer<Option> > options_t;
    options_t options;

    const std::string name;
    bool hidden;

  public:
    OptionCategory(const std::string &name, bool hidden = false) :
      name(name), hidden(hidden) {}

    options_t::const_iterator begin() const {return options.begin();}
    options_t::const_iterator end()   const {return options.end();}

    const std::string &getName() const {return name;}

    bool isEmpty() const {return options.empty();}
    bool hasSetOption() const;

    void setHidden(bool x) {hidden = x;}
    bool isHidden() const {return hidden;}

    void add(const SmartPointer<Option> &option);
    bool remove(const std::string &key);

    void write(JSON::Sink &sink, bool config = false) const;
    void write(XML::Handler &handler, uint32_t flags) const;
    void printHelp(std::ostream &stream, bool cmdLine = false) const;
  };


  using OptionCatPtr = SmartPointer<OptionCategory>;
}
