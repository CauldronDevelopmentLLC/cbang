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

#include "ProxySink.h"


namespace cb {
  namespace JSON {
    class YAMLMergeSink : public ProxySink {
      SmartPointer<Sink> target;
      bool list = false;

    public:
      YAMLMergeSink(const SmartPointer<Sink> &target) :
        ProxySink(target), target(target) {}

      const SmartPointer<Sink> &getTarget() const {return target;}

      bool inRoot() const;
      bool inRootDict() const;

      // From Sink
      void writeNull() override;
      void writeBoolean(bool value) override;
      void write(double value) override;
      void write(int8_t value) override;
      void write(uint8_t value) override;
      void write(int16_t value) override;
      void write(uint16_t value) override;
      void write(int32_t value) override;
      void write(uint32_t value) override;
      void write(int64_t value) override;
      void write(uint64_t value) override;
      void write(const std::string &value) override;

      // List functions
      void beginList(bool simple) override;
      void beginAppend() override;
      void endList() override;

      // Dict functions
      void beginDict(bool simple) override;
      bool has(const std::string &key) const override;
      void beginInsert(const std::string &key) override;
      void endDict() override;

    protected:
      void assertNotInRoot() const;
    };
  }
}
