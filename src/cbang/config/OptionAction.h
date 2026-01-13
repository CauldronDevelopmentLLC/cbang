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

#include <cbang/Exception.h>


namespace cb {
  class Option;

  /// The non-templated base class of OptionAction.
  // TODO replace with std::function.
  class OptionActionBase {
  public:
    virtual ~OptionActionBase() {}
    virtual int operator()(Option &) = 0;
  };


  class BareOptionActionBase : public OptionActionBase {
  public:
    virtual int operator()() = 0;

    // From OptionActionBase
    int operator()(Option &option) override {return operator()();}
  };


  /**
   * Used to execute an fuction when a command line option is encountered.
   * This makes it possible to load a configuration file in the middle of
   * command line processing.  Thus making the options overridable by
   * subsequent options.
   */
  template <typename T>
  struct BareOptionAction : public BareOptionActionBase {
    typedef int (T::*member_t)();
    T *obj;
    member_t member;

    BareOptionAction(T *obj, member_t member) : obj(obj), member(member) {
      if (!obj) CBANG_THROW("Object cannot be NULL");
      if (!member) CBANG_THROW("Member cannot be NULL");
    }

    int operator()() override {return (*obj.*member)();}
  };

  template <typename T>
  struct OptionAction : public OptionActionBase {
  typedef int (T::*member_t)(Option &);
    T *obj;
    member_t member;

    OptionAction(T *obj, member_t member) : obj(obj), member(member) {
      if (!obj) CBANG_THROW("Object cannot be NULL");
      if (!member) CBANG_THROW("Member cannot be NULL");
    }

    int operator()(Option &option) override {return (*obj.*member)(option);}
  };
}
