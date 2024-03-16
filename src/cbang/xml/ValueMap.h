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

#include "Handler.h"

#include <string>
#include <iostream>


namespace cb {
  namespace XML {
    class Writer;

    class ValueMap : public Handler {
      const std::string root;

      std::string filename;
      std::string xmlValue;
      bool xmlValueSet;
      unsigned depth;

    public:
      ValueMap(const std::string &root);

      void setFilename(const std::string &x) {filename = x;}

      void read(const std::string &filename);
      virtual void read(std::istream &stream);
      void write(const std::string &filename) const;
      virtual void write(std::ostream &stream) const;

      void writeXMLValue(Writer &writer, const std::string &name,
                         const std::string &value) const;

      virtual void writeXMLValues(Writer &writer) const = 0;
      virtual void setXMLValue(const std::string &name,
                               const std::string &value) = 0;

      // From Handler
      void startElement(
        const std::string &name, const Attributes &attrs) override;
      void endElement(const std::string &name) override;
      void text(const std::string &text) override;
      void cdata(const std::string &data) override;
    };

    inline std::istream &operator>>(std::istream &stream, ValueMap &o) {
      o.read(stream);
      return stream;
    }

    inline std::ostream &operator<<(std::ostream &stream, const ValueMap &o) {
      o.write(stream);
      return stream;
    }
  }
}
