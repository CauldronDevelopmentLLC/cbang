/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include <cbang/SmartPointer.h>
#include <cbang/socket/SocketType.h>

#include <iostream>


namespace cb {
  class Pipe {
  public:
    typedef socket_t handle_t;

  private:
    bool toChild;
    handle_t handles[2];
    bool closeHandles[2];
    SmartPointer<std::iostream> stream;

  public:
    explicit Pipe(bool toChild);

    handle_t getHandle(bool childEnd) const;
    handle_t getParentHandle() const {return getHandle(false);}
    handle_t getChildHandle()  const {return getHandle(true);}

    void closeHandle(bool childEnd);
    void closeParentHandle() {closeHandle(false);}
    void closeChildHandle()  {closeHandle(true);}

    void setBlocking(bool blocking, bool childEnd);
    void setSize(int size, bool childEnd);

    void create();
    void close();

    const SmartPointer<std::iostream> &getStream();
    void closeStream();

    void inChildProc(int target = -1);
  };
}
