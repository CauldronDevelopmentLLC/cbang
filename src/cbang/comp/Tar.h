/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "TarHeader.h"
#include "Compression.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <iostream>

namespace cb {
  class Tar : public TarHeader, public Compression {
  public:
    static const char zero_block[512];
    unsigned bufferSize;
    SmartPointer<char>::Array buf;

    Tar(unsigned bufferSize = 1024 * 1024);

    /// NOTE: Automatically calls writeHeader()
    unsigned writeFile(const std::string &filename, std::ostream &dst,
                       std::istream &src, uint32_t mode = 0666);
    /// NOTE: Automatically calls writeHeader()
    unsigned writeFile(const std::string &filename, std::ostream &dst,
                       const char *data, std::streamsize size,
                       uint32_t mode = 0666);
    unsigned writeFileData(std::ostream &dst, std::istream &src,
                           std::streamsize size);
    unsigned writeFileData(std::ostream &dst, const char *data,
                           std::streamsize size);
    unsigned writeDir(const std::string &name, std::ostream &dst,
                      uint32_t mode = 0777);
    unsigned writeFooter(std::ostream &dst);

    bool readHeader(std::istream &stream) {return TarHeader::read(stream);}
    void writeHeader(std::ostream &stream) {TarHeader::write(stream);}

    /// NOTE: You must call readHeader() and check TarHeader::isEOF()
    void readFile(std::ostream &dst, std::istream &src);
    /// NOTE: You must call readHeader() and check TarHeader::isEOF()
    void skipFile(std::istream &src);
  };
}
