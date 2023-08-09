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
#include "Value.h"
#include "Factory.h"

#include <vector>
#include <functional>


namespace cb {
  namespace JSON {
    class Builder : public Factory, public Sink {
      std::vector<ValuePtr> stack;
      bool appendNext;
      std::string nextKey;

    public:
      Builder(const ValuePtr &root = 0);

      static ValuePtr build(std::function<void (Sink &sink)> cb);

      ValuePtr getRoot() const;
      void clear() {stack.clear();}

      // From Sink
      void writeNull() override;
      void writeBoolean(bool value) override;
      void write(double value) override;
      void write(uint64_t value) override;
      void write(int64_t value) override;
      void write(const std::string &value) override;
      using Sink::write;
      void beginList(bool simple = false) override;
      void beginAppend() override;
      void endList() override;
      void beginDict(bool simple = false) override;
      bool has(const std::string &key) const override;
      void beginInsert(const std::string &key) override;
      void endDict() override;

    protected:
      void add(const ValuePtr &value);
      void assertNotPending();
      bool shouldAppend();
      bool shouldInsert() {return !nextKey.empty();}
    };


    static inline ValuePtr build(std::function<void (Sink &sink)> cb) {
      return JSON::Builder::build(cb);
    }
  }
}
