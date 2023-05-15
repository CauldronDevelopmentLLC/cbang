/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "SubprocessPool.h"
#include "Base.h"
#include "Event.h"

#include <cbang/Catch.h>
#include <cbang/os/SystemInfo.h>

#include <signal.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


SubprocessPool::SubprocessPool(Base &base) :
  maxActive(SystemInfo::instance().getCPUCount()) {

  execEvent = base.newEvent(this, &SubprocessPool::exec);
  if (base.hasPriorities()) execEvent->setPriority(8);
  execEvent->add();

  signalEvent = base.newSignal(SIGCHLD, this, &SubprocessPool::childSignal);
  if (base.hasPriorities()) signalEvent->setPriority(7);
  signalEvent->add();
}


void SubprocessPool::enqueue(const SmartPointer<AsyncSubprocess> &proc) {
  if (quit) THROW("Shutting down");
  procs.push(proc);
  if (active.size() < maxActive) execEvent->activate();
}


void SubprocessPool::shutdown() {
  quit = true;
  while (!procs.empty()) procs.pop();
  active.clear();
}


void SubprocessPool::exec() {
  if (quit) return;

  while (active.size() < maxActive && !procs.empty()) {
    SmartPointer<AsyncSubprocess> proc = procs.top();
    procs.pop();

    try {
      proc->exec();
      active.push_back(proc);
      LOG_DEBUG(5, "Launched process " << proc->getPID());
      continue;
    } CATCH_ERROR;

    // An error occured
    proc->done();
  }
}


void SubprocessPool::childSignal() {
  if (quit) return;

  for (active_t::iterator it = active.begin(); it != active.end();) {
    AsyncSubprocess &proc = **it;

    try {
      proc.wait(true);

      if (!proc.isRunning()) {
        if (proc.getWasKilled())
          LOG_WARNING("Process " << proc.getPID() << " was killed");

        else if (proc.getDumpedCore())
          LOG_WARNING("Process " << proc.getPID() << " dumped core");

        else if (proc.getReturnCode())
          LOG_WARNING("Process " << proc.getPID() << " returned "
                      << proc.getReturnCode());

        LOG_DEBUG(5, "Reaping PID " << proc.getPID());

        TRY_CATCH_ERROR(proc.done());
        if (!procs.empty()) execEvent->activate();

        it = active.erase(it);
        continue;
      }

    } CATCH_ERROR;

    it++;
  }
}
