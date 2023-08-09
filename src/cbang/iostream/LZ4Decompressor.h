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

#include <cbang/SmartPointer.h>
#include <cbang/boost/IOStreams.h>

#include <cstring>

#include <lz4/lz4frame.h>


namespace cb {
  class LZ4Decompressor {
    class LZ4DecompressorImpl {
      LZ4F_dctx *ctx = 0;

      std::streamsize capacity = 4096;
      std::streamsize fill = 0;
      char *buffer = 0;
      bool done = false;

    public:
      LZ4DecompressorImpl() : buffer(new char[capacity]) {
        auto err = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
        if (LZ4F_isError(err))
          CBANG_THROW("LZ4 error: " << LZ4F_getErrorName(err));
      }


      ~LZ4DecompressorImpl() {
        if (buffer) delete [] buffer;
        if (ctx) LZ4F_freeDecompressionContext(ctx);
      }


      template<typename Source>
      std::streamsize read(Source &src, char *s, std::streamsize n) {
        if (!n) return 0;

        std::streamsize bytes = 0;

        while (n) {
          // Read some data
          std::streamsize space = capacity - fill;
          if (space && !done) {
            std::streamsize count = io::read(src, buffer + fill, space);
            if (count == -1) done = true;
            else fill += count;
          }

          if (!fill) break;

          // Decompress
          size_t bytesIn = fill;
          size_t bytesOut = n;
          LZ4F_decompress(ctx, s, &bytesOut, buffer, &bytesIn, 0);

          s += bytesOut;
          n -= bytesOut;
          bytes += bytesOut;

          fill -= bytesIn;
          if (fill) memmove(buffer, buffer + bytesIn, fill);
        }

        return bytes;
      }


      template<typename Sink>
      std::streamsize write(Sink &dest, const char *s, std::streamsize n) {
        std::streamsize bytes = 0;

        while (n) {
          // Decompress
          size_t bytesIn = n;
          size_t bytesOut = capacity - fill;
          LZ4F_decompress(ctx, buffer + fill, &bytesOut, s, &bytesIn, 0);

          s += bytesIn;
          n -= bytesIn;
          bytes += bytesIn;
          fill += bytesOut;

          // Write data
          if (fill) {
            std::streamsize count = io::write(dest, buffer, fill);
            fill -= count;
            if (fill) memmove(buffer, buffer + count, fill);
          }
        }

        return bytes;
      }


      template<typename Sink> void close(Sink &dest, BOOST_IOS::openmode m) {
        if (!(m & BOOST_IOS::out)) return;

        if (fill) {
          std::streamsize count = io::write(dest, buffer, fill);
          if (count < fill)
            CBANG_THROW("Failed to write final LZ4 decompression data");
        }
     }
    };

    SmartPointer<LZ4DecompressorImpl> impl;


  public:
    typedef char char_type;
    struct category :
      io::dual_use, io::filter_tag, io::multichar_tag, io::closable_tag {};


    LZ4Decompressor() : impl(new LZ4DecompressorImpl) {}


    template<typename Source>
    std::streamsize read(Source &src, char *s, std::streamsize n) {
      return impl->read(src, s, n);
    }


    template<typename Sink>
    std::streamsize write(Sink &dest, const char *s, std::streamsize n) {
      return impl->write(dest, s, n);
    }


    template<typename Sink> void close(Sink &dest, BOOST_IOS::openmode m) {
      impl->close(dest, m);
    }
  };
}
