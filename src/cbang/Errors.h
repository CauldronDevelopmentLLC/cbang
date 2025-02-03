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

#pragma once

#include "Throw.h"

namespace cb {
  CBANG_DEFINE_EXCEPTION_SUBCLASS(KeyError);
  CBANG_DEFINE_EXCEPTION_SUBCLASS(TypeError);
  CBANG_DEFINE_EXCEPTION_SUBCLASS(CastError);
  CBANG_DEFINE_EXCEPTION_SUBCLASS(SystemError);
  CBANG_DEFINE_EXCEPTION_SUBCLASS(IOError);
  CBANG_DEFINE_EXCEPTION_SUBCLASS(ReferenceError);
  CBANG_DEFINE_EXCEPTION_SUBCLASS(ParseError);
  CBANG_DEFINE_EXCEPTION_SUBCLASS(NotImplementedError);
};


#define CBANG_KEY_ERROR(MSG) CBANG_THROWT(cb::KeyError, MSG)
#define CBANG_TYPE_ERROR(MSG) CBANG_THROWT(cb::TypeError, MSG)
#define CBANG_CAST_ERROR() CBANG_THROWT(cb::CastError, "Invalid Cast")
#define CBANG_SYSTEM_ERROR(MSG) CBANG_THROWT(cb::SystemError, MSG)
#define CBANG_IO_ERROR(MSG) CBANG_THROWT(cb::IOError, MSG)
#define CBANG_REFERENCE_ERROR(MSG) CBANG_THROWT(cb::ReferenceError, MSG)
#define CBANG_PARSE_ERROR(MSG) CBANG_THROWT(cb::ParseError, MSG)
#define CBANG_NOT_IMPLEMENTED_ERROR() \
  CBANG_THROWT(cb::NotImplementedError, "Not implemented")


#ifdef USING_CBANG
#define KEY_ERROR(MSG)          CBANG_KEY_ERROR(MSG)
#define TYPE_ERROR(MSG)         CBANG_TYPE_ERROR(MSG)
#define CAST_ERROR()            CBANG_CAST_ERROR()
#define SYSTEM_ERROR(MSG)       CBANG_SYSTEM_ERROR(MSG)
#define IO_ERROR(MSG)           CBANG_IO_ERROR(MSG)
#define REFERENCE_ERROR(MSG)    CBANG_REFERENCE_ERROR(MSG)
#define PARSE_ERROR(MSG)        CBANG_PARSE_ERROR(MSG)
#define NOT_IMPLEMENTED_ERROR() CBANG_NOT_IMPLEMENTED_ERROR()
#endif // USING_CBANG
