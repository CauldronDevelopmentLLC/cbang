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

#ifndef CBANG_ENUM
#ifndef CBANG_FDPOOL_EPOLL_COMMAND_H
#define CBANG_FDPOOL_EPOLL_COMMAND_H

#define CBANG_ENUM_NAME FDPoolEPollCommand
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 Event
#define CBANG_ENUM_PATH cbang/event
#define CBANG_ENUM_PREFIX 4
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_FDPOOL_EPOLL_COMMAND_H
#else // CBANG_ENUM

CBANG_ENUM(CMD_READ)
CBANG_ENUM(CMD_WRITE)
CBANG_ENUM(CMD_FLUSH)
CBANG_ENUM(CMD_FLUSHED)
CBANG_ENUM(CMD_COMPLETE)
CBANG_ENUM(CMD_READ_PROGRESS)
CBANG_ENUM(CMD_WRITE_PROGRESS)
CBANG_ENUM(CMD_READ_SIZE)
CBANG_ENUM(CMD_WRITE_SIZE)
CBANG_ENUM(CMD_READ_FINISHED)
CBANG_ENUM(CMD_WRITE_FINISHED)
CBANG_ENUM(CMD_STATUS)

#endif // CBANG_ENUM
