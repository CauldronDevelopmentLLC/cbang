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
  class PipeEnd {
  public:
    typedef socket_t handle_t;

  private:
    handle_t handle;

  public:
    PipeEnd(handle_t handle = -1) : handle(handle) {}

    operator handle_t() const {return handle;}
    handle_t getHandle() const {return handle;}
    void setHandle(handle_t handle) {handle = handle;}

    bool isOpen() const {return handle != -1;}

    void close();
    void setBlocking(bool blocking);
    void setSize(int size);
    bool moveFD(int target);
    SmartPointer<std::iostream> toStream();
  };


  class Pipe {
    bool toChild;
    PipeEnd ends[2];

  public:
    explicit Pipe(bool toChild) : toChild(toChild) {}

    PipeEnd &getReadEnd()  {return ends[0];}
    PipeEnd &getWriteEnd() {return ends[1];}

    PipeEnd &getParentEnd() {return toChild ? getWriteEnd() : getReadEnd();}
    PipeEnd &getChildEnd()  {return toChild ? getReadEnd()  : getWriteEnd();}

    void create();
    void close();
  };
}
