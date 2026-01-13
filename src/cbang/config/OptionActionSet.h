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

#include "OptionAction.h"
#include "Option.h"

#include <cbang/String.h>
#include <cbang/Exception.h>

namespace cb {
  template <typename T>
  class OptionActionSet : public OptionActionBase {
    T &ref;

  public:
    OptionActionSet(T &ref) : ref(ref) {}

    int operator()(Option &option) override {
      set(*option.get());
      return 0;
    }

    void set(const JSON::Value &value) {ref = T::parse(value.asString());}
  };

  template <>
  inline void OptionActionSet<std::string>::set(const JSON::Value &value) {
    ref = value.asString();
  }

  template <>
  inline void OptionActionSet<uint8_t>::set(const JSON::Value &value) {
    ref = value.getU8();
  }

  template <>
  inline void OptionActionSet<int8_t>::set(const JSON::Value &value) {
    ref = value.getS8();
  }

  template <>
  inline void OptionActionSet<uint16_t>::set(const JSON::Value &value) {
    ref = value.getU16();
  }

  template <>
  inline void OptionActionSet<int16_t>::set(const JSON::Value &value) {
    ref = value.getS16();
  }

  template <>
  inline void OptionActionSet<uint32_t>::set(const JSON::Value &value) {
    ref = value.getU32();
  }

  template <>
  inline void OptionActionSet<int32_t>::set(const JSON::Value &value) {
    ref = value.getS32();
  }

  template <>
  inline void OptionActionSet<uint64_t>::set(const JSON::Value &value) {
    ref = value.getU64();
  }

  template <>
  inline void OptionActionSet<int64_t>::set(const JSON::Value &value) {
    ref = value.getS64();
  }

  template <>
  inline void OptionActionSet<double>::set(const JSON::Value &value) {
    ref = value.getNumber();
  }

  template <>
  inline void OptionActionSet<float>::set(const JSON::Value &value) {
    ref = (float)value.getNumber();
  }

  template <>
  inline void OptionActionSet<bool>::set(const JSON::Value &value) {
    ref = value.getBoolean();
  }
}
