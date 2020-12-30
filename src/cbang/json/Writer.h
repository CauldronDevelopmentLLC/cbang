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

#include "NullSink.h"

#include <vector>
#include <ostream>


namespace cb {
  namespace JSON {
    class Writer : public NullSink {
    protected:
      std::ostream &stream;

      unsigned indentSpace;
      unsigned indentStart;
      bool compact;
      int precision;

      std::vector<bool> simple;
      bool first = true;

    public:
      Writer(std::ostream &stream, unsigned indentStart = 0,
             bool compact = false, unsigned indentSpace = 2, int precision = 6,
             bool allowDuplicates = false)
        : NullSink(allowDuplicates), stream(stream), indentSpace(indentSpace),
          indentStart(indentStart), compact(compact), precision(precision) {}

      unsigned getIndentSpace() const {return indentSpace;}
      void setIndentSpace(unsigned x) {indentSpace = x;}

      unsigned getIndentStart() const {return indentStart;}
      void setIndentStart(unsigned x) {indentStart = x;}

      bool getCompact() const {return compact;}
      void setCompact(bool x) {compact = x;}

      int getPrecision() const {return precision;}
      void setPrecision(int x) {precision = x;}

      // From NullSink
      void close();
      void reset();

      // From Sink
      void writeNull();
      void writeBoolean(bool value);
      void write(double value);
      void write(uint64_t value);
      void write(int64_t value);
      void write(const std::string &value);
      void beginList(bool simple = false);
      void beginAppend();
      void endList();
      void beginDict(bool simple = false);
      void beginInsert(const std::string &key);
      void endDict();

      static std::string escape(const std::string &s);

    protected:
      void indent() const;
    };
  }
}
