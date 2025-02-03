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

#include <cbang/SmartPointer.h>

#include <streambuf>


namespace cb {
  class StreamBuf : public std::streambuf {
    int fd = -1;

    static const int bufferSize = 4096;
    SmartPointer<char>::Array readBuf;
    SmartPointer<char>::Array writeBuf;

  public:
    StreamBuf(
      const std::string &path, std::ios::openmode mode, int perm);
    ~StreamBuf() {close();}

  protected:
    void close();

    int_type underflow() override;
    int_type overflow(int_type c) override;
    int sync() override;
    std::streampos seekoff(std::streamoff off, std::ios::seekdir way,
                           std::ios::openmode which) override;
    std::streampos seekpos(std::streampos sp,
                           std::ios::openmode which) override;
  };
}
