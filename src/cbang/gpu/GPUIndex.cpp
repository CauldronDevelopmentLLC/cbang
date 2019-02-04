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

#include "GPUIndex.h"

#include <cbang/String.h>


using namespace std;
using namespace cb;


GPU GPUIndex::find(uint16_t vendorID, uint16_t deviceID) const {
  gpus_t::const_iterator it = gpus.find(GPU(vendorID, deviceID));
  return it == gpus.end() ? GPU() : *it;
}


void GPUIndex::add(const GPU &gpu) {
  gpus.erase(gpu); // Remove any previous entry
  gpus.insert(gpu);
}


void GPUIndex::read(istream &stream) {
  while (stream.good()) {
    // Read line
    char line[4096];
    stream.getline(line, 4096);
    if (!stream.good()) break;

    // Parse line
    vector<string> tokens;
    String::tokenize(line, tokens, ":", true);
    if (tokens.size() != 5) THROWS("Invalid GPUs.txt");

    uint16_t vendorID = String::parseU16(tokens[0]);
    uint16_t deviceID = String::parseU16(tokens[1]);
    uint16_t type = tokens[2].empty() ? 0 : String::parseU16(tokens[2]);
    uint16_t species = tokens[3].empty() ? 0 : String::parseU16(tokens[3]);
    string &name = tokens[4];

    add(GPU(vendorID, deviceID, name, type, species));
  }
}


void GPUIndex::write(ostream &stream) const {
  for (gpus_t::const_iterator it = gpus.begin(); it != gpus.end(); it++)
    stream << it->toRowString() << '\n';
}
