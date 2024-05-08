/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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
#include <iostream>
#include <cstdint>


// https://www.gnu.org/software/tar/manual/html_node/Standard.html

namespace cb {
  struct TarHeader {
    enum type_t {
      REG_FILE        = 0,
      NORMAL_FILE     = '0',
      HARD_LINK       = '1',
      SYMBOLIC_LINK   = '2',
      CHARACTER_DEV   = '3',
      BLOCK_DEV       = '4',
      DIRECTORY       = '5',
      FIFO            = '6',
      CONTIGUOUS_FILE = '7',
      PAX_EXTENDED    = 'x',
      PAX_GLOBAL      = 'g',
    };

    char filename[100];
    char mode[8];
    char owner[8];
    char group[8];
    char size[12];
    char mod_time[12];
    char checksum[8];
    char type[1];
    char link_name[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char reserved[12];

    bool checksum_valid;

    TarHeader(const std::string &filename = "", uint64_t size = 0);

    unsigned updateChecksum();

    bool read(std::istream &stream);
    void write(std::ostream &stream);

    void setFilename(const std::string &filename);
    void setMode(uint32_t mode);
    void setOwner(uint32_t owner);
    void setGroup(uint32_t group);
    void setSize(uint64_t size);
    void setModTime(uint64_t mod_time);
    void setType(type_t type);
    void setLinkName(const std::string &link_name);

    const std::string getFilename() const;
    uint32_t getMode() const;
    uint32_t getOwner() const;
    uint32_t getGroup() const;
    uint64_t getSize() const;
    uint64_t getModTime() const;
    type_t getType() const;
    const std::string getLinkName() const;

    bool isEOF() const;

    static void writeNumber(uint32_t n, char *buf, unsigned length);
    static void writeNumber(uint64_t n, char *buf, unsigned length);
    static void writeString(const std::string &s, char *buf, unsigned length);
    static uint64_t readNumber(const char *buf, unsigned length);

  protected:
    unsigned computeChecksum();
  };
}
