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

#include "BZip2.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cstring>

#include <bzlib.h>

using namespace cb;
using namespace std;

#define BUFFER_SIZE (64 * 1024)

struct BZip2::Private : public bz_stream {
  Private() {clear();}
  void clear() {memset(this, 0, sizeof(bz_stream));}
};


BZip2::BZip2(ostream &out) :
  out(out), bz(new Private), init(false), comp(true) {}


void BZip2::compressInit(int blockSize100k, int verbosity, int workFactor) {
  if (init) THROW("BZip2: Already initialized");
  init = true;
  comp = true;
  BZ2_bzCompressInit(bz.get(), blockSize100k, verbosity, workFactor);
}


void BZip2::decompressInit(int verbosity, int small) {
  if (init) THROW("BZip2: Already initialized");
  init = true;
  comp = false;
  BZ2_bzDecompressInit(bz.get(), verbosity, small);
}


void BZip2::update(const char *data, unsigned length) {
  if (!init) THROW("BZip2: Not initialized");

  char buf[BUFFER_SIZE];

  bz->next_in = const_cast<char *>(data);
  bz->avail_in = length;

  while (true) {
    bz->next_out = buf;
    bz->avail_out = BUFFER_SIZE;

    if (comp) {
      int ret = BZ2_bzCompress(bz.get(), BZ_RUN);
      if (ret != BZ_RUN_OK)
        THROW("BZip2: Compression failed " << errorStr(ret));

    } else {
      int ret = BZ2_bzDecompress(bz.get());
      if (ret == BZ_STREAM_END) bz->avail_in = 0;
      else if (ret != BZ_OK)
        THROW("BZip2: Decompression failed " << errorStr(ret));
    }

    out.write(buf, BUFFER_SIZE - bz->avail_out);
    if (out.fail()) THROW("BZip2: Output stream failed");
    if (!bz->avail_in) break;
  }
}


void BZip2::update(const std::string &s) {
  update(CPP_TO_C_STR(s), s.length());
}


void BZip2::read(std::istream &in) {
  char buf[BUFFER_SIZE];

  while (!in.fail() && !out.fail()) {
    in.read(buf, BUFFER_SIZE);
    update(buf, in.gcount());
  }
}


void BZip2::finish() {
  if (!init) THROW("BZip2: Not initialized");
  int ret;

  if (comp) {
    char buf[BUFFER_SIZE];

    bz->next_in = 0;
    bz->avail_in = 0;

    do {
      bz->next_out = buf;
      bz->avail_out = BUFFER_SIZE;
      ret = BZ2_bzCompress(bz.get(), BZ_FINISH);
      out.write(buf, BUFFER_SIZE - bz->avail_out);
    } while (ret == BZ_FINISH_OK);

    if (ret != BZ_STREAM_END)
      THROW("BZip2: compress finish " << errorStr(ret));
  }

  if (comp) ret = BZ2_bzCompressEnd(bz.get());
  else ret = BZ2_bzDecompressEnd(bz.get());

  if (ret != BZ_OK) THROW("BZip2: end failed " << errorStr(ret));

  bz->clear();
  init = false;
}


void BZip2::compress(istream &in, ostream &out, int blockSize100k,
                     int verbosity, int workFactor) {
  BZip2 bzip2(out);
  bzip2.compressInit(blockSize100k, verbosity, workFactor);
  bzip2.read(in);
  bzip2.finish();
}


void BZip2::decompress(istream &in, ostream &out, int verbosity, int small) {
  BZip2 bzip2(out);
  bzip2.decompressInit(verbosity, small);
  bzip2.read(in);
  bzip2.finish();
}


string BZip2::compress(const string &s, int blockSize100k, int verbosity,
                       int workFactor)  {
  ostringstream str;
  BZip2 bzip2(str);
  bzip2.compressInit(blockSize100k, verbosity, workFactor);
  bzip2.update(s);
  bzip2.finish();
  return str.str();
}


string BZip2::decompress(const string &s, int verbosity, int small) {
  ostringstream str;
  BZip2 bzip2(str);
  bzip2.decompressInit(verbosity, small);
  bzip2.update(s);
  bzip2.finish();
  return str.str();
}


const char *BZip2::errorStr(int err) {
  switch (err) {
  case BZ_OK:               return "OK";
  case BZ_RUN_OK:           return "RUN OK";
  case BZ_FLUSH_OK:         return "FLUSH OK";
  case BZ_FINISH_OK:        return "FINISH OK";
  case BZ_STREAM_END:       return "STREAM END";
  case BZ_SEQUENCE_ERROR:   return "SEQUENCE ERROR";
  case BZ_PARAM_ERROR:      return "PARAM ERROR";
  case BZ_MEM_ERROR:        return "MEM ERROR";
  case BZ_DATA_ERROR:       return "DATA ERROR";
  case BZ_DATA_ERROR_MAGIC: return "DATA ERROR MAGIC";
  case BZ_IO_ERROR:         return "IO ERROR";
  case BZ_UNEXPECTED_EOF:   return "UNEXPECTED EOF";
  case BZ_OUTBUFF_FULL:     return "OUTBUFF FULL";
  case BZ_CONFIG_ERROR:     return "CONFIG ERROR";
  default:                  return "UNKNOWN ERROR";
  }
}
