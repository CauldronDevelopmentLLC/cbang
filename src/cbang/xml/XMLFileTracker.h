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

#include <set>
#include <vector>
#include <string>

#include "XMLHandler.h"

namespace cb {
  class XMLFileTracker : public XMLHandler {
    typedef std::set<std::string> files_t;
    files_t files;
    typedef std::vector<files_t::const_iterator> stack_t;
    stack_t stack;

  public:
    bool hasFile() const {return !stack.empty();}
    const std::string &getCurrentFile();

    // From XMLHandler
    void pushFile(const std::string &filename);
    void popFile();
    void startElement(const std::string &name, const XMLAttributes &attrs) {}
    void endElement(const std::string &name) {}
    void text(const std::string &text) {}
    void cdata(const std::string &cdata) {}
  };
}
