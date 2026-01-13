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

#pragma once

#include <cbang/SmartPointer.h>
#include <cbang/boost/IOStreams.h>

#include <lz4frame.h>


namespace cb {
  class LZ4Compressor {
    class LZ4CompressorImpl {
      LZ4F_cctx *ctx = 0;

      std::streamsize capacity = 4096;
      std::streamsize fill = 0;
      char *buffer = 0;

    public:
      LZ4CompressorImpl() : buffer(new char[capacity]) {
        auto err = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
        if (LZ4F_isError(err))
          CBANG_THROW("LZ4 error: " << LZ4F_getErrorName(err));

        fill += LZ4F_compressBegin(ctx, buffer, capacity, 0);
      }


      ~LZ4CompressorImpl() {
        if (buffer) delete [] buffer;
        if (ctx) LZ4F_freeCompressionContext(ctx);
      }


      void reserve(std::streamsize space) {
        if (capacity - fill < space) {
          capacity = fill + space;
          char *newBuf = new char[capacity];
          memcpy(newBuf, buffer, fill);
          delete [] buffer;
          buffer = newBuf;
        }
      }


      template<typename Source>
      std::streamsize read(Source &src, char *s, std::streamsize n) {
        if (!n) return 0;

        while (fill < n) {
          char in[4096];
          auto bytes = io::read(src, in, 4096);

          // Check for EOF
          if (bytes < 0) {
            std::streamsize space = LZ4F_compressBound(0, 0);
            reserve(space);
            fill += LZ4F_compressEnd(ctx, buffer + fill, space, 0);
            break;
          }

          // Make sure we have enough space to compress data
          std::streamsize space = LZ4F_compressBound(bytes, 0);
          reserve(space);

          // Compress
          fill += LZ4F_compressUpdate(ctx, buffer + fill, space, in, bytes, 0);
        }

        std::streamsize bytes = fill < n ? fill : n;
        if (bytes) {
          memcpy(s, buffer, bytes);
          if (bytes < fill) memmove(buffer, buffer + fill, fill - bytes);
          fill -= bytes;
        }

        return bytes;
      }


      template<typename Sink>
      std::streamsize write(Sink &dest, const char *s, std::streamsize n) {
        // Make sure we have enough space to compress data
        std::streamsize space = LZ4F_compressBound(n, 0);
        reserve(space);

        // Compress
        fill += LZ4F_compressUpdate(ctx, buffer + fill, space, s, n, 0);

        // Write compressed data
        std::streamsize bytes = io::write(dest, buffer, fill);
        if (bytes < fill) memmove(buffer, buffer + bytes, fill - bytes);
        fill -= bytes;

        return n;
      }


      template<typename Sink>
      void close(Sink &dest, BOOST_IOS::openmode m) {
        if (!(m & BOOST_IOS::out)) return;

        // Make sure we have enough space
        std::streamsize space = LZ4F_compressBound(0, 0);
        reserve(space);

        // End compression
        fill += LZ4F_compressEnd(ctx, buffer + fill, space, 0);

        std::streamsize bytes = io::write(dest, buffer, fill);
        if (bytes < fill)
          CBANG_THROW("Failed to write final LZ4 compression data");
      }
    };


    SmartPointer<LZ4CompressorImpl> impl;

  public:
    typedef char char_type;
    struct category :
      io::dual_use, io::filter_tag, io::multichar_tag, io::closable_tag {};


    LZ4Compressor() : impl(new LZ4CompressorImpl) {}


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
