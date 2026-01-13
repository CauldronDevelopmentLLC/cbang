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

#include "Handler.h"

#include <iostream>


namespace cb {
  namespace XML {
    class Writer : public Handler {
      std::ostream &stream;
      bool pretty;

      bool closed;
      bool startOfLine;
      bool dontEscapeText;
      unsigned depth;

    public:
      Writer(std::ostream &stream, bool pretty = false) :
        stream(stream), pretty(pretty), closed(true), startOfLine(true),
        dontEscapeText(false), depth(0) {}

      void setPretty(bool x) {pretty = x;}
      bool getPretty() const {return pretty;}

      void setDontEscapeText(bool x = true) {dontEscapeText = x;}
      bool getDontEscapeText() const {return dontEscapeText;}

      void entityRef(const std::string &name);

      // From Handler
      void startElement(const std::string &name,
                        const Attributes &attrs = Attributes()) override;
      void endElement(const std::string &name) override;
      void text(const std::string &text) override;
      void cdata(const std::string &cdata) override;
      void comment(const std::string &text) override;

      void indent();
      void wrap();
      void simpleElement(const std::string &name,
                         const std::string &content = std::string(),
                         const Attributes &attrs = Attributes());

      static const std::string escape(const std::string &name);
    };
  }
}
