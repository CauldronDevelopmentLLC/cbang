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

#ifndef CBANG_ENUM
#ifndef CBANG_WS_STATUS_H
#define CBANG_WS_STATUS_H

#define CBANG_ENUM_NAME WebsockStatus
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 Event
#define CBANG_ENUM_PATH cbang/event
#define CBANG_ENUM_PREFIX 10
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_WS_STATUS_H
#else // CBANG_ENUM

CBANG_ENUM_VALUE(WS_STATUS_NORMAL,       1000)
CBANG_ENUM_VALUE(WS_STATUS_GOING_AWAY,   1001)
CBANG_ENUM_VALUE(WS_STATUS_PROTOCOL,     1002)
CBANG_ENUM_VALUE(WS_STATUS_UNACCEPTABLE, 1003)
CBANG_ENUM_VALUE(WS_STATUS_NONE,         1005)
CBANG_ENUM_VALUE(WS_STATUS_DIRTY_CLOSE,  1006)
CBANG_ENUM_VALUE(WS_STATUS_INCONSISTENT, 1007)
CBANG_ENUM_VALUE(WS_STATUS_VIOLATION,    1008)
CBANG_ENUM_VALUE(WS_STATUS_TOO_BIG,      1009)
CBANG_ENUM_VALUE(WS_STATUS_MISSING_EXTN, 1010)
CBANG_ENUM_VALUE(WS_STATUS_UNEXPECTED,   1011)
CBANG_ENUM_VALUE(WS_STATUS_TLS_FAILED,   1015)

#endif // CBANG_ENUM
