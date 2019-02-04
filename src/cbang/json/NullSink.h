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

#include "Sink.h"
#include "ValueType.h"

#include <vector>
#include <set>


namespace cb {
  namespace JSON {
    class NullSink : public Sink {
    protected:
      std::vector<ValueType> stack;
      typedef std::set<std::string> keys_t;
      std::vector<keys_t> keyStack;

      bool canWrite;

    public:
      NullSink() : canWrite(true) {}

      unsigned getDepth() const;
      bool inList() const;
      bool inDict() const;
      void end();

      virtual void close();
      virtual void reset();

      // From Sink
      void writeNull() {assertCanWrite();}
      void writeBoolean(bool value) {assertCanWrite();}
      void write(double value) {assertCanWrite();}
      void write(const std::string &value) {assertCanWrite();}
      void beginList(bool simple = false);
      void beginAppend();
      void endList();
      void beginDict(bool simple = false);
      bool has(const std::string &key) const;
      void beginInsert(const std::string &key);
      void endDict();

    protected:
      void assertCanWrite();
      void assertWriteNotPending();
    };
  }
}
