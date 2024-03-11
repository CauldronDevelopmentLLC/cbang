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

#ifndef CBANG_ENUM
#ifndef CBANG_DNS_ERROR_H
#define CBANG_DNS_ERROR_H

#define CBANG_ENUM_NAME Error
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 DNS
#define CBANG_ENUM_PATH cbang/dns
#define CBANG_ENUM_PREFIX 8
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_DNS_ERROR_H
#else // CBANG_ENUM

CBANG_ENUM_VALUE(DNS_ERR_NOERROR,      0)
CBANG_ENUM_VALUE(DNS_ERR_FORMAT,       1)
CBANG_ENUM_VALUE(DNS_ERR_SERVERFAILED, 2)
CBANG_ENUM_VALUE(DNS_ERR_NOTEXIST,     3)
CBANG_ENUM_VALUE(DNS_ERR_NOTIMPL,      4)
CBANG_ENUM_VALUE(DNS_ERR_REFUSED,      5)
CBANG_ENUM_VALUE(DNS_ERR_YXDOMAIN,     6)
CBANG_ENUM_VALUE(DNS_ERR_YXRRSET,      7)
CBANG_ENUM_VALUE(DNS_ERR_NXRRSET,      8)
CBANG_ENUM_VALUE(DNS_ERR_NOTAUTH,      9)
CBANG_ENUM_VALUE(DNS_ERR_NOTZONE,     10)
CBANG_ENUM_VALUE(DNS_ERR_DSOTYPENI,   11)
CBANG_ENUM_VALUE(DNS_ERR_BADSIG,      16)
CBANG_ENUM_VALUE(DNS_ERR_BADKEY,      17)
CBANG_ENUM_VALUE(DNS_ERR_BADTIME,     18)
CBANG_ENUM_VALUE(DNS_ERR_BADMODE,     19)
CBANG_ENUM_VALUE(DNS_ERR_BADNAME,     20)
CBANG_ENUM_VALUE(DNS_ERR_BADALG,      21)
CBANG_ENUM_VALUE(DNS_ERR_BADTRUNC,    22)
CBANG_ENUM_VALUE(DNS_ERR_BADCOOKIE,   23)

// Unofficial codes from libevent
CBANG_ENUM_VALUE(DNS_ERR_TRUNCATED,   65)
CBANG_ENUM_VALUE(DNS_ERR_UNKNOWN,     66)
CBANG_ENUM_VALUE(DNS_ERR_TIMEOUT,     67)
CBANG_ENUM_VALUE(DNS_ERR_SHUTDOWN,    68)
CBANG_ENUM_VALUE(DNS_ERR_CANCEL,      69)
CBANG_ENUM_VALUE(DNS_ERR_NODATA,      70)

// Unoffical codes from C!
CBANG_ENUM_VALUE(DNS_ERR_NOSERVER,   100)
CBANG_ENUM_VALUE(DNS_ERR_BADREQ,     101)

#endif // CBANG_ENUM
