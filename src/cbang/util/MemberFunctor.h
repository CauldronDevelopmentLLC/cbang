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

#include "Functor.h"


#define CBANG_MEMBER_FUNCTOR_(CLASS, BASE, RETURN, CALLBACK, CONST, ...) \
  template <typename T>                                                 \
  class CLASS : public BASE {                                           \
  public:                                                               \
    typedef RETURN (T::*member_t)(__VA_ARGS__) CONST;                   \
  protected:                                                            \
    T *obj;                                                             \
    member_t member;                                                    \
  public:                                                               \
    CLASS(T *obj, member_t member) : obj(obj), member(member) {         \
      if (!obj) CBANG_THROW("Object cannot be NULL");                   \
      if (!member) CBANG_THROW("Member cannot be NULL");                \
    }                                                                   \
    RETURN CALLBACK(CBANG_ARGS(__VA_ARGS__)) CONST {                    \
      return (*obj.*member)(CBANG_PARAMS(__VA_ARGS__));                 \
    }                                                                   \
  }


#define CBANG_MEMBER_FUNCTOR(CLASS, BASE, RETURN, CALLBACK, ...) \
  CBANG_MEMBER_FUNCTOR_(CLASS, BASE, RETURN, CALLBACK, , __VA_ARGS__)

#define CBANG_CONST_MEMBER_FUNCTOR(CLASS, BASE, RETURN, CALLBACK, ...)  \
  CBANG_MEMBER_FUNCTOR_(CLASS, BASE, RETURN, CALLBACK, const, __VA_ARGS__)
