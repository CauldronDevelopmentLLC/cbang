/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#ifndef CBANG_ENUM_EXPAND
#ifndef CBANG_HTTP_METHOD_H
#define CBANG_HTTP_METHOD_H

#define CBANG_ENUM_NAME Method
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 HTTP
#define CBANG_ENUM_PATH cbang/http
#define CBANG_ENUM_PREFIX 5 // The length of the HTTP_ prefix
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_HTTP_METHOD_H
#else // CBANG_ENUM_EXPAND

CBANG_ENUM_EXPAND(HTTP_UNKNOWN,      0)
CBANG_ENUM_EXPAND(HTTP_ANY,         ~0)
CBANG_ENUM_EXPAND(HTTP_OPTIONS, 1 << 0)
CBANG_ENUM_EXPAND(HTTP_GET,     1 << 1)
CBANG_ENUM_EXPAND(HTTP_HEAD,    1 << 2)
CBANG_ENUM_EXPAND(HTTP_POST,    1 << 3)
CBANG_ENUM_EXPAND(HTTP_PUT,     1 << 4)
CBANG_ENUM_EXPAND(HTTP_DELETE,  1 << 5)
CBANG_ENUM_EXPAND(HTTP_TRACE,   1 << 6)
CBANG_ENUM_EXPAND(HTTP_CONNECT, 1 << 7)
CBANG_ENUM_EXPAND(HTTP_PATCH,   1 << 8)

#endif // CBANG_ENUM_EXPAND
