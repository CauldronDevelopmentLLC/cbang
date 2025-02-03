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

#include <string>
#include <cstdint>


namespace cb {
  class Base64 {
    const unsigned width;
    char encodeTable[65];
    signed char decodeTable[256];

    static const char *_encodeTable;
    static const signed char _decodeTable[256];

    Base64(unsigned width);

  public:
    Base64(char pad, char a, char b, unsigned width = 0);
    Base64(const char *pad = "=", const char *a = "+-", const char *b = "/_",
           unsigned width = 0);

    unsigned getWidth() const {return width;}

    std::string encode(const std::string &s) const;
    std::string encode(const char *s, unsigned length) const;
    std::string decode(const std::string &s) const;
    std::string decode(const char *s, unsigned length) const;

  protected:
    char getPad() const;
    char encode(int x) const;
    int decode(char x) const;
  };


  class URLBase64 : public Base64 {
  public:
    URLBase64() : Base64("\0=", "-+", "_/") {}
  };
}
