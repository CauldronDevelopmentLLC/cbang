/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2017, Cauldron Development LLC
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

#include "MemberFunctor.h"

#if 199711L < __cplusplus

#include <functional>


#define CBANG_FUNCTION_(CLASS, BASE, RETURN, CALLBACK, CONST, ...)       \
  class CLASS : public BASE {                                           \
  public:                                                               \
    typedef std::function<RETURN (CBANG_ARGS(__VA_ARGS__))> func_t;     \
  protected:                                                            \
    func_t cb;                                                          \
  public:                                                               \
    CLASS(func_t cb) : cb(cb) {}                                        \
    RETURN CALLBACK(CBANG_ARGS(__VA_ARGS__)) CONST {                    \
      return cb(CBANG_PARAMS(__VA_ARGS__));                             \
    }                                                                   \
  }

#else // 199711L < __cplusplus

#define CBANG_FUNCTION_(...)

#endif // 199711L < __cplusplus


#define CBANG_FUNCTION(CLASS, BASE, RETURN, CALLBACK, ...)      \
  CBANG_FUNCTION_(CLASS, BASE, RETURN, CALLBACK, , __VA_ARGS__)

#define CBANG_CONST_FUNCTION(CLASS, BASE, RETURN, CALLBACK, ...)        \
  CBANG_FUNCTION_(CLASS, BASE, RETURN, CALLBACK, const, __VA_ARGS__)
