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
#ifndef CBANG_DNS_TYPE_H
#define CBANG_DNS_TYPE_H

#define CBANG_ENUM_NAME Type
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 DNS
#define CBANG_ENUM_PATH cbang/dns
#define CBANG_ENUM_PREFIX 4
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_DNS_TYPE_H
#else // CBANG_ENUM

CBANG_ENUM_VALUE(DNS_A,          1)
CBANG_ENUM_ALIAS(DNS_IPV4,   DNS_A)
CBANG_ENUM_VALUE(DNS_NS,         2)
CBANG_ENUM_VALUE(DNS_MD,         3)
CBANG_ENUM_VALUE(DNS_MF,         4)
CBANG_ENUM_VALUE(DNS_CNAME,      5)
CBANG_ENUM_VALUE(DNS_SOA,        6)
CBANG_ENUM_VALUE(DNS_MB,         7)
CBANG_ENUM_VALUE(DNS_MG,         8)
CBANG_ENUM_VALUE(DNS_MR,         9)
CBANG_ENUM_VALUE(DNS_NULL,       10)
CBANG_ENUM_VALUE(DNS_WKS,        11)
CBANG_ENUM_VALUE(DNS_PTR,        12)
CBANG_ENUM_VALUE(DNS_HINFO,      13)
CBANG_ENUM_VALUE(DNS_MINFO,      14)
CBANG_ENUM_VALUE(DNS_MX,         15)
CBANG_ENUM_VALUE(DNS_TXT,        16)
CBANG_ENUM_VALUE(DNS_RP,         17)
CBANG_ENUM_VALUE(DNS_AFSDB,      18)
CBANG_ENUM_VALUE(DNS_X25,        19)
CBANG_ENUM_VALUE(DNS_ISDN,       20)
CBANG_ENUM_VALUE(DNS_RT,         21)
CBANG_ENUM_VALUE(DNS_NSAP,       22)
CBANG_ENUM_VALUE(DNS_NSAP_PTR,   23)
CBANG_ENUM_VALUE(DNS_SIG,        24)
CBANG_ENUM_VALUE(DNS_KEY,        25)
CBANG_ENUM_VALUE(DNS_PX,         26)
CBANG_ENUM_VALUE(DNS_GPOS,       27)
CBANG_ENUM_VALUE(DNS_AAAA,       28)
CBANG_ENUM_ALIAS(DNS_IPV6, DNS_AAAA)
CBANG_ENUM_VALUE(DNS_LOC,        29)
CBANG_ENUM_VALUE(DNS_NXT,        30)
CBANG_ENUM_VALUE(DNS_EID,        31)
CBANG_ENUM_VALUE(DNS_NIMLOC,     32)
CBANG_ENUM_VALUE(DNS_SRV,        33)
CBANG_ENUM_VALUE(DNS_ATMA,       34)
CBANG_ENUM_VALUE(DNS_NAPTR,      35)
CBANG_ENUM_VALUE(DNS_KX,         36)
CBANG_ENUM_VALUE(DNS_CERT,       37)
CBANG_ENUM_VALUE(DNS_A6,         38)
CBANG_ENUM_VALUE(DNS_DNAME,      39)
CBANG_ENUM_VALUE(DNS_SINK,       40)
CBANG_ENUM_VALUE(DNS_OPT,        41)
CBANG_ENUM_VALUE(DNS_APL,        42)
CBANG_ENUM_VALUE(DNS_DS,         43)
CBANG_ENUM_VALUE(DNS_SSHFP,      44)
CBANG_ENUM_VALUE(DNS_IPSECKEY,   45)
CBANG_ENUM_VALUE(DNS_RRSIG,      46)
CBANG_ENUM_VALUE(DNS_NSEC,       47)
CBANG_ENUM_VALUE(DNS_DNSKEY,     48)
CBANG_ENUM_VALUE(DNS_DHCID,      49)
CBANG_ENUM_VALUE(DNS_NSEC3,      50)
CBANG_ENUM_VALUE(DNS_NSEC3PARAM, 51)
CBANG_ENUM_VALUE(DNS_TLSA,       52)
CBANG_ENUM_VALUE(DNS_SMIMEA,     53)
CBANG_ENUM_VALUE(DNS_HIP,        55)
CBANG_ENUM_VALUE(DNS_NINFO,      56)
CBANG_ENUM_VALUE(DNS_RKEY,       57)
CBANG_ENUM_VALUE(DNS_TALINK,     58)
CBANG_ENUM_VALUE(DNS_CDS,        59)
CBANG_ENUM_VALUE(DNS_CDNSKEY,    60)
CBANG_ENUM_VALUE(DNS_OPENPGPKEY, 61)
CBANG_ENUM_VALUE(DNS_CSYNC,      62)
CBANG_ENUM_VALUE(DNS_ZONEMD,     63)
CBANG_ENUM_VALUE(DNS_SVCB,       64)
CBANG_ENUM_VALUE(DNS_HTTPS,      65)
CBANG_ENUM_VALUE(DNS_SPF,        99)
CBANG_ENUM_VALUE(DNS_UINFO,      100)
CBANG_ENUM_VALUE(DNS_UID,        101)
CBANG_ENUM_VALUE(DNS_GID,        102)
CBANG_ENUM_VALUE(DNS_UNSPEC,     103)
CBANG_ENUM_VALUE(DNS_NID,        104)
CBANG_ENUM_VALUE(DNS_L32,        105)
CBANG_ENUM_VALUE(DNS_L64,        106)
CBANG_ENUM_VALUE(DNS_LP,         107)
CBANG_ENUM_VALUE(DNS_EUI48,      108)
CBANG_ENUM_VALUE(DNS_EUI64,      109)
CBANG_ENUM_VALUE(DNS_TKEY,       249)
CBANG_ENUM_VALUE(DNS_TSIG,       250)
CBANG_ENUM_VALUE(DNS_IXFR,       251)
CBANG_ENUM_VALUE(DNS_AXFR,       252)
CBANG_ENUM_VALUE(DNS_MAILB,      253)
CBANG_ENUM_VALUE(DNS_MAILA,      254)
CBANG_ENUM_VALUE(DNS_STAR,       255)
CBANG_ENUM_VALUE(DNS_URI,        256)
CBANG_ENUM_VALUE(DNS_CAA,        257)
CBANG_ENUM_VALUE(DNS_AVC,        258)
CBANG_ENUM_VALUE(DNS_DOA,        259)
CBANG_ENUM_VALUE(DNS_AMTRELAY,   260)
CBANG_ENUM_VALUE(DNS_RESINFO,    261)
CBANG_ENUM_VALUE(DNS_TA,         32768)
#endif
