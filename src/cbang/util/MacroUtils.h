/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#define CBANG_CONCAT(prefix, name) CBANG__CONCAT(prefix, name)
#define CBANG__CONCAT(prefix, name) prefix##name

#define CBANG_STRING(str) CBANG__STRING(str)
#define CBANG__STRING(str) #str

#define CBANG_CAT(a, ...) a ## __VA_ARGS__

#define CBANG_FIRST(a, ...) a
#define CBANG_SECOND(a, b, ...) b

#define CBANG_IS_PROBE(...) CBANG_SECOND(__VA_ARGS__, 0)
#define CBANG_PROBE() ~, 1

#define CBANG_NOT(x) CBANG_IS_PROBE(CBANG_CAT(CBANG__NOT_, x))
#define _CBANG_NOT_0 CBANG_PROBE()

#define CBANG_BOOL(x) CBANG_NOT(CBANG_NOT(x))

#define CBANG_IF( c) CBANG__IF(c)
#define CBANG__IF(c) CBANG_CAT(CBANG__IF_, c)
#define CBANG__IF_0(...)
#define CBANG__IF_1(...) __VA_ARGS__

#define CBANG_IF_ELSE( cond, t, f) CBANG__IF_ELSE(cond, t, f)
#define CBANG__IF_ELSE(cond, t, f) CBANG__IF_ELSE_ ## cond(t, f)
#define CBANG__IF_ELSE_0(t, f) f
#define CBANG__IF_ELSE_1(t, f) t

#define CBANG_COMMA() ,
