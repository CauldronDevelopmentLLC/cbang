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

#include "FD.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/Progress.h>

#include <functional>


namespace cb {
  namespace Event {
    class Base;

    class FDPool : public RefCounted {
    protected:
      FDPool() {}

    public:
      virtual ~FDPool() {}

      static SmartPointer<FDPool> create(Base &base);

      virtual const Rate &getReadRate() const = 0;
      virtual const Rate &getWriteRate() const = 0;
      virtual Progress getReadProgress(int fd) const = 0;
      virtual Progress getWriteProgress(int fd) const = 0;
      virtual void read(const SmartPointer<Transfer> &t) = 0;
      virtual void write(const SmartPointer<Transfer> &t) = 0;
      virtual void flush(int fd, std::function <void ()> cb) = 0;
    };
  }
}
