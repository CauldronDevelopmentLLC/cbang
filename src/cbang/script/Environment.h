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

#include "Entity.h"
#include "Function.h"
#include "Variable.h"
#include "MemberFunctor.h"
#include "BareMemberFunctor.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/FormatCheck.h>

#include <map>

namespace cb {
  namespace Script {
    class Environment :
      public Handler, protected std::map<std::string, SmartPointer<Entity> > {
    protected:
      Handler *parent;
      const std::string name;

    public:
      Environment(const Environment &env);
      Environment(const std::string &name, Handler *parent = 0);

      const std::string &getName() const {return name;}

      const SmartPointer<Entity> &add(const SmartPointer<Entity> &e);

      template <class T> SmartPointer<Entity>
      add(const std::string &name, T *obj,
          typename Script::MemberFunctor<T>::member_t member,
          unsigned minArgs = 0, unsigned maxArgs = 0,
          const std::string &help = "",
          const std::string &argHelp = "",
          bool autoEvalArgs = true) {
        return add(new Script::MemberFunctor<T>
                   (name, obj, member, minArgs, maxArgs, help, argHelp,
                    autoEvalArgs));
      }

      template <class T> SmartPointer<Entity>
      add(const std::string &name, T *obj,
          typename Script::BareMemberFunctor<T>::member_t member,
          unsigned minArgs = 0, unsigned maxArgs = 0,
          const std::string &help = "",
          const std::string &argHelp = "",
          bool autoEvalArgs = true) {
        return add(new Script::BareMemberFunctor<T>
                   (name, obj, member, minArgs, maxArgs, help, argHelp,
                    autoEvalArgs));
      }

      void set(const std::string &name, const std::string &value);
      void setf(const char *key, const char *value, ...)
        FORMAT_CHECK(printf, 3, 4);
      void unset(const std::string &name) {erase(name);}

      // From Handler
      using Handler::eval;
      bool eval(const Context &ctx) override;

      virtual void localEval(const Context &ctx, Entity &e);

      void evalHelp(const Context &ctx);
      void evalSet(const Context &ctx);
      void evalUnset(const Context &ctx);
    };
  }
}
