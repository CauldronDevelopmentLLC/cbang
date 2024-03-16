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

#pragma once

#include <cbang/SmartPointer.h>

#include <iostream>


namespace cb {
  class BZip2 {
    struct Private;

    std::ostream &out;
    SmartPointer<Private> bz;
    bool init;
    bool comp;

  public:
    BZip2(std::ostream &out);

    void compressInit(int blockSize100k = 9, int verbosity = 0,
                      int workFactor = 0);
    void decompressInit(int verbosity = 0, int small = 0);
    void update(const char *data, unsigned length);
    void update(const std::string &s);
    void read(std::istream &in);
    void finish();

    BZip2 &operator<<(const std::string &s) {update(s); return *this;}

    static void compress(std::istream &in, std::ostream &out,
                         int blockSize100k = 9, int verbosity = 0,
                         int workFactor = 0);
    static void decompress(std::istream &in, std::ostream &out,
                           int verbosity = 0, int small = 0);
    static std::string compress(const std::string &s, int blockSize100k = 9,
                                int verbosity = 0, int workFactor = 0);
    static std::string decompress(const std::string &s, int verbosity = 0,
                                  int small = 0);

    static const char *errorStr(int err);
  };
}
