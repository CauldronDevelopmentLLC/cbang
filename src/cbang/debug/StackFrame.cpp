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

#include "StackFrame.h"

#include <cbang/String.h>
#include <cbang/json/Sink.h>

#include <exception>
#include <cstdint>
#include <cinttypes>

using namespace std;
using namespace cb;


string StackFrame::getAddrString() const {
  return String::printf("0x%08" PRIxPTR, (uintptr_t)addr);
}


const string &StackFrame::getFunction() const {
  if (!location) throw std::runtime_error("StackFrame not resolved");
  return location->getFunction();
}


ostream &StackFrame::print(ostream &stream) const {
  stream << getAddrString();

  if (location) {
    if (!location->getFunction().empty())
      stream << " in " << location->getFunction();

    if (!location->getFilename().empty())
      stream << " at " << location->getFileLineColumn();
  }

  return stream;
}


void StackFrame::write(JSON::Sink &sink) const {
  sink.beginDict();

  sink.insert("address", getAddrString());

  if (location && !location->isEmpty()) {
    sink.beginInsert("location");
    location->write(sink);
  }

  sink.endDict();
}
