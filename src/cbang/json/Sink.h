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

#include <cbang/Exception.h>

#include <string>


namespace cb {
  namespace JSON {
    class Value;

    class Sink {
    public:
      virtual ~Sink() {}

      // Element functions
      virtual void writeNull() = 0;
      virtual void writeBoolean(bool value) = 0;
      virtual void write(double value) = 0;
      virtual void write(int8_t value) {write((int32_t)value);}
      virtual void write(uint8_t value) {write((uint32_t)value);}
      virtual void write(int16_t value) {write((int32_t)value);}
      virtual void write(uint16_t value) {write((uint32_t)value);}
      virtual void write(int32_t value) {write((int64_t)value);}
      virtual void write(uint32_t value) {write((uint64_t)value);}
      virtual void write(int64_t value) {write((double)value);}
      virtual void write(uint64_t value) {write((double)value);}
      virtual void write(const std::string &value) = 0;
      void write(const Value &value);

      // List functions
      virtual void beginList(bool simple = false) = 0;
      virtual void beginAppend() = 0;
      virtual void endList() = 0;

      // Dict functions
      virtual void beginDict(bool simple = false) = 0;
      virtual bool has(const std::string &key) const = 0;
      virtual void beginInsert(const std::string &key) = 0;
      virtual void endDict() = 0;

      // List functions
      void appendNull() {beginAppend(); writeNull();}
      void appendBoolean(bool value) {beginAppend(); writeBoolean(value);}
      template <typename T>
      void append(const T &value) {beginAppend(); write(value);}
      void appendList(bool simple = false) {beginAppend(); beginList(simple);}
      void appendDict(bool simple = false) {beginAppend(); beginDict(simple);}
      void append(const Value &value);

      // Dict functions
      void insertNull(const std::string &key) {beginInsert(key); writeNull();}
      void insertBoolean(const std::string &key, bool value)
      {beginInsert(key); writeBoolean(value);}
      template <typename T> void insert(const std::string &key, const T &value)
      {beginInsert(key); write(value);}
      virtual void insertList(const std::string &key, bool simple = false)
      {beginInsert(key); beginList(simple);}
      virtual void insertDict(const std::string &key, bool simple = false)
      {beginInsert(key); beginDict(simple);}
      void insert(const std::string &key, const Value &value);
    };
  }
}
