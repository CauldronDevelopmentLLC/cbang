/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#define CBANG_ENUM_IMPL
#include "Compression.h"
#include <cbang/enum/MakeEnumerationImpl.def>
#include <cbang/String.h>

using namespace cb;
using namespace std;


namespace cb {
  Compression compressionFromPath(const string &path) {
    if (String::endsWith(path, ".bz2") || String::endsWith(path, ".bzip2"))
      return Compression::COMPRESSION_BZIP2;
    if (String::endsWith(path, ".zlib")) return Compression::COMPRESSION_ZLIB;
    if (String::endsWith(path, ".gz") || String::endsWith(path, ".gzip"))
      return Compression::COMPRESSION_GZIP;
    if (String::endsWith(path, ".lz4")) return Compression::COMPRESSION_LZ4;

    return Compression::COMPRESSION_NONE;
  }


  const char *compressionExtension(Compression compression) {
    switch (compression) {
    case Compression::COMPRESSION_BZIP2: return ".bz2";
    case Compression::COMPRESSION_ZLIB:  return ".zlib";
    case Compression::COMPRESSION_GZIP:  return ".gz";
    case Compression::COMPRESSION_LZ4:   return ".lz4";
    case Compression::COMPRESSION_NONE:  return "";
    case Compression::COMPRESSION_AUTO:  break;
    }

    THROW("Invalid compression type " << (unsigned)compression);
  }
}
