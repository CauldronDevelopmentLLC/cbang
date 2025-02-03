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
#ifndef CBANG_HTTP_STATUS_H
#define CBANG_HTTP_STATUS_H

#define CBANG_ENUM_NAME Status
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 HTTP
#define CBANG_ENUM_PATH cbang/http
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_HTTP_STATUS_H
#else // CBANG_ENUM

CBANG_ENUM_VALUE(HTTP_UNKNOWN,                         0)

CBANG_ENUM_VALUE(HTTP_CONTINUE,                        100)
CBANG_ENUM_VALUE(HTTP_SWITCHING_PROTOCOLS,             101)
CBANG_ENUM_VALUE(HTTP_PROCESSING,                      102)
CBANG_ENUM_VALUE(HTTP_EARLY_HINTS,                     103)

CBANG_ENUM_VALUE(HTTP_OK,                              200)
CBANG_ENUM_VALUE(HTTP_CREATED,                         201)
CBANG_ENUM_VALUE(HTTP_ACCEPTED,                        202)
CBANG_ENUM_VALUE(HTTP_NON_AUTHORITATIVE_INFORMATION,   203)
CBANG_ENUM_VALUE(HTTP_NO_CONTENT,                      204)
CBANG_ENUM_VALUE(HTTP_RESET_CONTENT,                   205)
CBANG_ENUM_VALUE(HTTP_PARTIAL_CONTENT,                 206)
CBANG_ENUM_VALUE(HTTP_MULTI_STATUS,                    207)
CBANG_ENUM_VALUE(HTTP_ALREADY_REPORTED,                208)
CBANG_ENUM_VALUE(HTTP_IM_USED,                         226)

CBANG_ENUM_VALUE(HTTP_MULTIPLE_CHOICES,                300)
CBANG_ENUM_VALUE(HTTP_MOVED_PERMANENTLY,               301)
CBANG_ENUM_VALUE(HTTP_FOUND,                           302)
CBANG_ENUM_VALUE(HTTP_SEE_OTHER,                       303)
CBANG_ENUM_VALUE(HTTP_NOT_MODIFIED,                    304)
CBANG_ENUM_VALUE(HTTP_USE_PROXY,                       305)
CBANG_ENUM_VALUE(HTTP_TEMPORARY_REDIRECT,              307)

CBANG_ENUM_VALUE(HTTP_BAD_REQUEST,                     400)
CBANG_ENUM_VALUE(HTTP_UNAUTHORIZED,                    401)
CBANG_ENUM_VALUE(HTTP_PAYMENT_REQUIRED,                402)
CBANG_ENUM_VALUE(HTTP_FORBIDDEN,                       403)
CBANG_ENUM_VALUE(HTTP_NOT_FOUND,                       404)
CBANG_ENUM_VALUE(HTTP_METHOD_NOT_ALLOWED,              405)
CBANG_ENUM_VALUE(HTTP_NOT_ACCEPTABLE,                  406)
CBANG_ENUM_VALUE(HTTP_PROXY_AUTHENTICATION_REQUIRED,   407)
CBANG_ENUM_VALUE(HTTP_REQUEST_TIME_OUT,                408)
CBANG_ENUM_VALUE(HTTP_CONFLICT,                        409)
CBANG_ENUM_VALUE(HTTP_GONE,                            410)
CBANG_ENUM_VALUE(HTTP_LENGTH_REQUIRED,                 411)
CBANG_ENUM_VALUE(HTTP_PRECONDITION_FAILED,             412)
CBANG_ENUM_VALUE(HTTP_REQUEST_ENTITY_TOO_LARGE,        413)
CBANG_ENUM_VALUE(HTTP_REQUEST_URI_TOO_LARGE,           414)
CBANG_ENUM_VALUE(HTTP_UNSUPPORTED_MEDIA_TYPE,          415)
CBANG_ENUM_VALUE(HTTP_REQUESTED_RANGE_NOT_SATISFIABLE, 416)
CBANG_ENUM_VALUE(HTTP_EXPECTATION_FAILED,              417)
CBANG_ENUM_VALUE(HTTP_IM_AT_TEAPOT,                    418)
CBANG_ENUM_VALUE(HTTP_MISDIRECTED_REQUEST,             421)
CBANG_ENUM_VALUE(HTTP_UNPROCESSABLE_ENTITY,            422)
CBANG_ENUM_VALUE(HTTP_LOCKED,                          423)
CBANG_ENUM_VALUE(HTTP_FAILED_DEPENDENCY,               424)
CBANG_ENUM_VALUE(HTTP_TOO_EARLY,                       425)
CBANG_ENUM_VALUE(HTTP_UPGRADE_REQUIRED,                426)
CBANG_ENUM_VALUE(HTTP_PRECONDITION_REQUIRED,           428)
CBANG_ENUM_VALUE(HTTP_TOO_MANY_REQUESTS,               429)
CBANG_ENUM_VALUE(HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE, 431)
CBANG_ENUM_VALUE(HTTP_UNAVAILABLE_FOR_LEGAL_REASONS,   451)

CBANG_ENUM_VALUE(HTTP_INTERNAL_SERVER_ERROR,           500)
CBANG_ENUM_VALUE(HTTP_NOT_IMPLEMENTED,                 501)
CBANG_ENUM_VALUE(HTTP_BAD_GATEWAY,                     502)
CBANG_ENUM_VALUE(HTTP_SERVICE_UNAVAILABLE,             503)
CBANG_ENUM_VALUE(HTTP_GATEWAY_TIME_OUT,                504)
CBANG_ENUM_VALUE(HTTP_VERSION_NOT_SUPPORTED,           505)
CBANG_ENUM_VALUE(HTTP_VARIANT_ALSO_NEGOTIATES,         506)
CBANG_ENUM_VALUE(HTTP_INSUFFICIENT_STORAGE,            507)
CBANG_ENUM_VALUE(HTTP_LOOP_DETECTED,                   508)
CBANG_ENUM_VALUE(HTTP_NOT_EXTENDED,                    510)
CBANG_ENUM_VALUE(HTTP_NETWORK_AUTHENTICATION_REQUIRED, 511)

#endif // CBANG_ENUM
