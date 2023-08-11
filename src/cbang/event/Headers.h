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

#pragma once

#include <cbang/String.h>
#include <cbang/util/OrderedDict.h>

#include <ostream>


namespace cb {
  namespace Event {
    class Buffer;

    struct HeaderKeyCompare {
      bool operator()(const std::string &a, const std::string &b) const {
        return String::toLower(a) < String::toLower(b);
      }
    };

    class Headers :
      public OrderedDict<std::string, std::string, HeaderKeyCompare> {
    public:
      std::string find(const std::string &key) const;
      void set(const std::string &key, const std::string &value)
        {insert(key, value);}
      void remove(const std::string &key);
      bool keyContains(const std::string &key, const std::string &value) const;

      bool hasContentType() const {return has("Content-Type");}
      std::string getContentType() const;
      void setContentType(const std::string &contentType);
      void guessContentType(const std::string &ext);
      bool needsClose() const;
      bool connectionKeepAlive() const;

      bool parse(Buffer &buf, unsigned maxSize = 0);
      void write(std::ostream &stream) const;
    };


    inline static
    std::ostream &operator<<(std::ostream &stream, const Headers &h) {
      h.write(stream);
      return stream;
    }
  }
}
