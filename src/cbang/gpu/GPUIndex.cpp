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

#include "GPUIndex.h"

#include <cbang/String.h>
#include <cbang/json/Value.h>


using namespace std;
using namespace cb;


bool GPUIndex::has(uint16_t vendorID, uint16_t deviceID) const {
  return gpus.find(GPU(vendorID, deviceID)) != gpus.end();
}


GPU GPUIndex::find(uint16_t vendorID, uint16_t deviceID) const {
  auto it = gpus.find(GPU(vendorID, deviceID));
  return it == gpus.end() ? GPU() : *it;
}


void GPUIndex::add(const GPU &gpu) {gpus.insert(gpu);}


void GPUIndex::read(const JSON::Value &value) {
  clear();
  for (unsigned i = 0; i < value.size(); i++)
    add(GPU(*value.get(i)));
}


void GPUIndex::write(JSON::Sink &sink) const {
  sink.beginList();

  for (auto it = gpus.begin(); it != gpus.end(); it++) {
    sink.beginAppend();
    it->write(sink);
  }

  sink.endList();
}
