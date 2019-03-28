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

#include <cbang/Errors.h>

namespace cb {
  namespace JSON {
    CBANG_DEFINE_EXCEPTION_SUBCLASS(Error);
    CBANG_DEFINE_EXCEPTION_SUPER(KeyError, cb::KeyError);
    CBANG_DEFINE_EXCEPTION_SUPER(TypeError, cb::TypeError);
    CBANG_DEFINE_EXCEPTION_SUPER(ParseError, cb::ParseError);
  }
}


#define CBANG_JSON_ERROR(MSG) CBANG_THROWT(cb::JSON::Error, MSG)
#define CBANG_JSON_KEY_ERROR(MSG) CBANG_THROWT(cb::JSON::KeyError, MSG)
#define CBANG_JSON_TYPE_ERROR(MSG) CBANG_THROWT(cb::JSON::TypeError, MSG)
#define CBANG_JSON_PARSE_ERROR(MSG) CBANG_THROWT(cb::JSON::ParseError, MSG)


#ifdef USING_CBANG
#define JSON_ERROR(MSG) CBANG_JSON_ERROR(MSG)
#define JSON_KEY_ERROR(MSG) CBANG_JSON_KEY_ERROR(MSG)
#define JSON_TYPE_ERROR(MSG) CBANG_JSON_TYPE_ERROR(MSG)
#define JSON_PARSE_ERROR(MSG) CBANG_JSON_PARSE_ERROR(MSG)
#endif // USING_CBANG
