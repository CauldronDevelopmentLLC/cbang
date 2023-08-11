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

#include "Header.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <set>
#include <iostream>
#include <cstdint>


namespace cb {
  namespace HTTP {
    class Message : public Header {
    public:
      static const char *TIME_FORMAT;

    protected:
      float version;

    public:
      Message(float version = 1.1) : version(version) {}
      virtual ~Message() {}

      void reset() {clear();}

      float getVersion() const {return version;}
      void setVersion(float version) {this->version = version;}

      bool isChunked() const;
      void setChunked(bool chunked);

      bool hasCookie(const std::string &name) const;
      std::string getCookie(const std::string &name) const;
      std::string findCookie(const std::string &name) const;

      void setPersistent(bool x);
      bool isPersistent() const;

      std::string getHeader() const;

      std::string getVersionString() const;
      void readVersionString(const std::string &s);

      bool isRequest() const {return !isResponse();}

      virtual bool isResponse() const = 0;
      virtual std::string getHeaderLine() const = 0;
      virtual void readHeaderLine(const std::string &line) = 0;

      void read(const std::string &data);
      virtual std::istream &read(std::istream &stream);
      virtual std::ostream &write(std::ostream &stream) const;
    };


    inline std::ostream &operator<<(std::ostream &stream, const Message &m) {
      return m.write(stream);
    }
  }
}
