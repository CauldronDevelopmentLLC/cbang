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

#include <string>
#include <vector>


namespace cb {
  namespace HTTP {
    // Parser for `multipart/form-data` request bodies (RFC 7578).  Binary-safe:
    // part data may contain any bytes, including NULs and CRLF.
    class MultipartParser {
    public:
      struct Part {
        std::string name;      // form field name from Content-Disposition
        std::string filename;  // empty unless a file part
        std::string type;      // Content-Type, empty if not given
        std::string data;      // raw bytes

        bool isFile() const {return !filename.empty();}
      };

      // Extract the boundary from a `multipart/form-data` Content-Type header
      // value.  Returns "" if the type is not multipart or has no boundary.
      static std::string getBoundary(const std::string &contentType);

      // Parse a multipart body.  Throws on malformed input.
      static std::vector<Part> parse(const std::string &body,
                                     const std::string &boundary);
    };
  }
}
