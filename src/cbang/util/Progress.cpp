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

#include "Progress.h"

#include <cbang/Catch.h>
#include <cbang/time/Timer.h>

#include <algorithm>

using namespace cb;


uint64_t Progress::getETA() const {
  double remaining = (1 - getProgress()) * size;
  double rate = getRate();
  return rate ? remaining / rate : 0;
}


double Progress::getProgress() const {
  if (!size) return 0;
  double progress = getTotal() / size;
  return std::max(0.0, std::min(1.0, progress));
}


void Progress::setCallback(callback_t cb, double cbRate) {
  this->cb = cb;
  this->cbRate = cbRate;
}


void Progress::onUpdate() {
  if (!cb) return;

  double now = Timer::now();

  if (getTotal() == size || (lastCB + cbRate) <= now)
    try {
      cb(*this);
      lastCB = now;
    } CATCH_ERROR;
}
