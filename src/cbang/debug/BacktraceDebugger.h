/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "Debugger.h"
#include <cbang/SmartPointer.h>

#include <string>
#include <map>


namespace cb {
  class BacktraceDebugger : public Debugger {
    struct private_t;
    private_t *p;

    std::string executable;
    bool initialized;
    bool enabled;

    typedef std::map<void *, SmartPointer<FileLocation> > cache_t;
    cache_t cache;

  public:
    int parents;

    BacktraceDebugger();
    ~BacktraceDebugger();

    static bool supported();
    void getStackTrace(StackTrace &trace, bool resolved) override;
    void resolve(StackTrace &trace) override;

  protected:
    const FileLocation &resolve(void *addr);
    void init();
    void release();
  };
}
