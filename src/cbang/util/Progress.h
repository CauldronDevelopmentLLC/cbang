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

#include "Rate.h"

#include <functional>


namespace cb {
  class Progress : public Rate {
  public:
    typedef std::function<void (const Progress &)> callback_t;

  protected:
    uint64_t size;
    uint64_t start;
    uint64_t end;

    callback_t cb;
    double lastCB = 0;
    double cbRate = 0;

  public:
    Progress(unsigned buckets = 60 * 5, unsigned period = 1, uint64_t size = 0,
      uint64_t start = Time::now(), uint64_t end = Time::now()) :
      Rate(buckets, period), size(size), start(start), end(end) {}

    void setSize(uint64_t size) {this->size = size;}
    uint64_t getSize() const {return size;}
    void setStart(uint64_t start) {this->start = start;}
    uint64_t getStart() const {return start;}
    void setEnd(uint64_t end) {this->end = end;}
    uint64_t getEnd() const {return end;}
    uint64_t getDuration() {return end - start;}
    uint64_t getETA() const;
    double getProgress() const;
    double getRate() const {return Rate::get(end);}

    void setCallback(callback_t cb, double cbRate = 0);

    // From Rate
    void onUpdate(bool force) override;
  };
}
