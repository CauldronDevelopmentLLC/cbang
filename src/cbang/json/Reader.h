/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "Value.h"

#include <cbang/io/InputSource.h>


namespace cb {
  namespace JSON {
    class Value;
    class List;
    class Dict;
    class Sink;

    class Reader {
      InputSource src;
      std::istream &stream;
      bool strict;

      unsigned line = 0;
      unsigned column = 0;

    public:
      Reader(const InputSource &src, bool strict = false) :
        src(src), stream(src.getStream()), strict(strict) {}

      bool getStrict() const {return strict;}
      void setStrict(bool strict) {this->strict = strict;}

      void parse(Sink &sink, unsigned depth = 0);
      ValuePtr parse();
      static ValuePtr parse(const InputSource &src, bool strict = false);
      static ValuePtr parseString(const std::string &s, bool strict = false);
      static void parse(const InputSource &src, Sink &sink,
                        bool strict = false);
      static void parseString(const std::string &s, Sink &sink,
                              bool strict = false);

      unsigned getLine() const {return line;}
      unsigned getColumn() const {return column;}

      char get();
      char next();
      void advance() {stream.get();}
      bool tryMatch(char c);
      char match(const char *chars);
      bool good() const {return stream.good();}

      const std::string parseKeyword();
      void parseNull();
      bool parseBoolean();
      void parseNumber(Sink &sink);
      std::string parseString();
      void parseList(Sink &sink, unsigned depth = 0);
      void parseDict(Sink &sink, unsigned depth = 0);

      void error(const std::string &msg) const;
    };


    static inline ValuePtr parse(const InputSource &src) {
      return JSON::Reader::parse(src);
    }

    static inline ValuePtr parseString(const std::string &s) {
      return JSON::Reader::parseString(s);
    }
  }
}
