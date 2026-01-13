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

#ifndef CBANG_ENUM
#ifndef CBANG_COMPRESSION_H
#define CBANG_COMPRESSION_H

#define CBANG_ENUM_NAME Compression
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PATH cbang/comp
#define CBANG_ENUM_PREFIX 12
#include <cbang/enum/MakeEnumeration.def>

#include <string>

namespace cb {
  Compression compressionFromPath(const std::string &path);
  const char *compressionExtension(Compression compression);
}

#endif // CBANG_COMPRESSION_H
#else // CBANG_ENUM

// NOTE, don't change these values
CBANG_ENUM_VALUE(COMPRESSION_NONE,  0)
CBANG_ENUM_VALUE(COMPRESSION_BZIP2, 1)
CBANG_ENUM_VALUE(COMPRESSION_ZLIB,  2)
CBANG_ENUM_VALUE(COMPRESSION_GZIP,  3)
CBANG_ENUM_VALUE(COMPRESSION_LZ4,   4)
CBANG_ENUM_VALUE(COMPRESSION_AUTO,  255)

#endif // CBANG_ENUM
