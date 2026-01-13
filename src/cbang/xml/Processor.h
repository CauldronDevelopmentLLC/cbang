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
#include "FileTracker.h"

#include <vector>


namespace cb {
  namespace XML {
    class HandlerContext;
    class HandlerFactory;

    class Processor : public Handler {
      typedef std::vector<HandlerContext *> context_stack_t;
      context_stack_t contextStack;

      FileTracker fileTracker;

    public:
      Processor();
      virtual ~Processor();

      const std::string &getCurrentFile() {return fileTracker.getCurrentFile();}

      virtual void addFactory(
        const std::string &name,HandlerFactory *factory);
      virtual HandlerFactory *getFactory(const std::string &name);

      virtual void pushContext();
      virtual void popContext();

      // From Handler
      void pushFile(const std::string &filename) override
        {fileTracker.pushFile(filename);}
      void popFile() override {fileTracker.popFile();}
    };
  }
}
