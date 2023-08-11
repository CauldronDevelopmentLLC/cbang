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

#pragma once

#include <cbang/enum/Compression.h>

#include <cbang/iostream/BZip2Compressor.h>
#include <cbang/iostream/BZip2Decompressor.h>
#include <cbang/iostream/LZ4Compressor.h>
#include <cbang/iostream/LZ4Decompressor.h>

#include <cbang/boost/StartInclude.h>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <cbang/boost/EndInclude.h>


namespace cb {
  template <typename T>
  static inline void pushCompression(Compression compression, T &filter) {
    switch (compression) {
    case Compression::COMPRESSION_NONE: return;
    case Compression::COMPRESSION_BZIP2:
      return filter.push(BZip2Compressor());
    case Compression::COMPRESSION_GZIP:
      return filter.push(io::gzip_compressor());
    case Compression::COMPRESSION_ZLIB:
      return filter.push(io::zlib_compressor());
    case Compression::COMPRESSION_LZ4:
      return filter.push(LZ4Compressor());
    case Compression::COMPRESSION_AUTO: break;
    }

    CBANG_THROW("Invalid compression type " << compression);
  }


  template <typename T>
  static inline void pushDecompression(Compression compression, T &filter) {
    switch (compression) {
    case Compression::COMPRESSION_NONE: return;
    case Compression::COMPRESSION_BZIP2:
      return filter.push(BZip2Decompressor());
    case Compression::COMPRESSION_GZIP:
      return filter.push(io::gzip_decompressor());
    case Compression::COMPRESSION_ZLIB:
      return filter.push(io::zlib_decompressor());
    case Compression::COMPRESSION_LZ4:
      return filter.push(LZ4Decompressor());
    case Compression::COMPRESSION_AUTO: break;
    }

    CBANG_THROW("Invalid compression type " << compression);
  }
}
