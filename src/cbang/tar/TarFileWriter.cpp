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

#include "TarFileWriter.h"

#include <cbang/os/SystemUtilities.h>

#include <cbang/iostream/CompressionFilter.h>

using namespace cb;
using namespace std;


struct TarFileWriter::private_t {
  io::filtering_ostream filter;
};


TarFileWriter::TarFileWriter(const string &path, ios::openmode mode, int perm,
                             Compression compression) :
  pri(new private_t),
  stream(SystemUtilities::open(path, mode | ios::out, perm)) {

  if (compression == COMPRESSION_AUTO) compression = compressionFromPath(path);
  pushCompression(compression, pri->filter);
  pri->filter.push(*this->stream);
}


TarFileWriter::TarFileWriter(ostream &stream, Compression compression) :
  pri(new private_t), stream(SmartPointer<ostream>::Phony(&stream)) {

  pushCompression(compression, pri->filter);
  pri->filter.push(*this->stream);
}


TarFileWriter::~TarFileWriter() {delete pri;}


void TarFileWriter::add(const string &path, const string &filename,
                        uint32_t mode) {
  uint64_t size = SystemUtilities::getFileSize(path);

  // TODO handle other file types (e.g. symlinks)

  add(*SystemUtilities::iopen(path), filename.empty() ? path : filename, size,
      mode);
}


void TarFileWriter::add(istream &in, const string &filename, uint64_t size,
                        uint32_t mode) {
  writeHeader(TarHeader::NORMAL_FILE, filename, size, mode);
  writeFileData(pri->filter, in, size);
}


void TarFileWriter::add(const char *data, size_t size, const string &filename,
                        uint32_t mode) {
  writeHeader(TarHeader::NORMAL_FILE, filename, size, mode);
  writeFileData(pri->filter, data, size);
}


void TarFileWriter::writeHeader(type_t type, const string &filename,
                                uint64_t size, uint32_t mode) {
  setType(type);
  setFilename(filename);
  setSize(size);
  setMode(mode);
  writeHeader(pri->filter);
}
