/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Pipe.h"
#include "ProcessPriority.h"

#include <cbang/util/StringMap.h>

#include <string>
#include <iostream>
#include <vector>


namespace cb {
  class Subprocess : public StringMap, public ProcessPriority::Enum {
  public:
    enum {
      SHELL                   = 1 << 0,
      REDIR_STDIN             = 1 << 1,
      REDIR_STDOUT            = 1 << 2,
      REDIR_STDERR            = 1 << 3,
      MERGE_STDOUT_AND_STDERR = 1 << 4,
      CLEAR_ENVIRONMENT       = 1 << 5,
      NULL_STDOUT             = 1 << 6,
      NULL_STDERR             = 1 << 7,
      CREATE_PROCESS_GROUP    = 1 << 8,
      W32_HIDE_WINDOW         = 1 << 9,
      W32_WAIT_FOR_INPUT_IDLE = 1 << 10,
      MAX_PIPE_SIZE           = 1 << 11,
      USE_VFORK               = 1 << 12,
    };

    enum {
      PROCESS_EXITED          = 1 << 0,
      PROCESS_SIGNALED        = 1 << 1,
      PROCESS_DUMPED_CORE     = 1 << 2,
    };

  protected:
    struct Private;
    Private *p;

    std::vector<Pipe> pipes;

    bool running     = false;
    bool signalGroup = false;
    int  returnCode  = 0;
    int  exitFlags   = 0;

    std::string wd;

  public:
    Subprocess();
    ~Subprocess();

    bool isRunning();
    uint64_t getPID() const;
    int getReturnCode()  const {return returnCode;}
    int getExitFlags()   const {return exitFlags;}
    bool getWasKilled()  const {return exitFlags & PROCESS_SIGNALED;}
    bool getDumpedCore() const {return exitFlags & PROCESS_DUMPED_CORE;}
    bool exitedOk() const
      {return !returnCode && !getWasKilled() && !getDumpedCore();}

    unsigned createPipe(bool toChild);

    PipeEnd &getPipe(unsigned i);
    PipeEnd &getPipeIn()  {return getPipe(0);}
    PipeEnd &getPipeOut() {return getPipe(1);}
    PipeEnd &getPipeErr() {return getPipe(2);}

    void setWorkingDirectory(const std::string &wd) {this->wd = wd;}

    void exec(const std::vector<std::string> &args, unsigned flags = 0,
              ProcessPriority priority = PRIORITY_INHERIT);
    void exec(const std::string &command, unsigned flags = 0,
              ProcessPriority priority = PRIORITY_INHERIT);

    bool kill(bool nonblocking = false);
    void interrupt();
    int wait(bool nonblocking = false);

    static void parse(const std::string &command,
                      std::vector<std::string> &args);
    static std::string assemble(const std::vector<std::string> &args);

    static unsigned priorityToClass(ProcessPriority priority);

  protected:
    void closeProcessHandles();
    void closePipes();
    void closeHandles();
  };
}
