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

#include "TarFileReader.h"
#include "CompressionFilter.h"

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SysError.h>
#include <cbang/log/Logger.h>

using namespace cb;
using namespace std;


struct TarFileReader::private_t {
  io::filtering_istream filter;
};


TarFileReader::TarFileReader(const string &path, Compression compression) :
  pri(new private_t), stream(SystemUtilities::iopen(path)),
  didReadHeader(false) {

  if (compression == COMPRESSION_AUTO) compression = compressionFromPath(path);
  pushDecompression(compression, pri->filter);
  pri->filter.push(*this->stream);
}


TarFileReader::TarFileReader(istream &stream, Compression compression) :
  pri(new private_t), stream(SmartPointer<istream>::Phony(&stream)),
  didReadHeader(false) {

  pushDecompression(compression, pri->filter);
  pri->filter.push(*this->stream);
}


TarFileReader::~TarFileReader() {delete pri;}


bool TarFileReader::hasMore() {
  if (!didReadHeader) {
    SysError::clear();
    if (!readHeader(pri->filter))
      THROW("Tar file read failed: " << SysError());
    didReadHeader = true;
  }

  return !isEOF();
}


bool TarFileReader::next() {
  if (didReadHeader) {
    skipFile(pri->filter);
    didReadHeader = false;
  }

  return hasMore();
}


std::string TarFileReader::extract(const string &_path) {
  if (_path.empty()) THROW("path cannot be empty");
  if (!hasMore()) THROW("No more tar files");

  string path = _path;
  if (SystemUtilities::isDirectory(path)) {
    path += "/" + getFilename();

    // Check that path is under the target directory
    string a = SystemUtilities::getCanonicalPath(_path);
    string b = SystemUtilities::getCanonicalPath(path);
    if (!String::startsWith(b, a + "/"))
      THROW("Tar path points outside of the extraction directory: " << path);
  }

  LOG_DEBUG(5, "Extracting: " << path);

  switch (getType()) {
  case REG_FILE: case NORMAL_FILE: case CONTIGUOUS_FILE:
    return extract(*SystemUtilities::oopen(path, getMode()));

  case DIRECTORY:
    SystemUtilities::ensureDirectory(path);
    didReadHeader = false;
    break;

  default: THROW("Unsupported tar file type " << getType());
  }

  return getFilename();
}


string TarFileReader::extract(ostream &out) {
  if (!hasMore()) THROW("No more tar files");

  readFile(out, pri->filter);
  didReadHeader = false;

  return getFilename();
}


void TarFileReader::extractAll(const string &path) {
  while (next()) extract(path);
}
