/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2017, Cauldron Development LLC
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

#include "AsyncWorkerFunc.h"

#include <cbang/SmartPointer.h>

#include <cbang/os/ThreadPool.h>
#include <cbang/os/Condition.h>

#include <cbang/util/SmartLock.h>
#include <cbang/util/SmartUnlock.h>
#include <cbang/Catch.h>

#include <queue>


namespace cb {
  class AsyncScheduler : public ThreadPool, private Condition {
    typedef std::queue<SmartPointer<AsyncWorker> > queue_t;
    queue_t ready;
    queue_t done;

  public:
    AsyncScheduler(unsigned size) : ThreadPool(size) {}


    void add(const SmartPointer<AsyncWorker> &worker) {
      SmartLock lock(this);
      ready.push(worker);
      Condition::signal();
    }


    void add(std::function<void ()> run, std::function<void ()> done = 0) {
      add(new AsyncWorkerFunc(run, done));
    }


    void reap() {
      SmartLock lock(this);

      while (!done.empty()) {
        SmartPointer<AsyncWorker> worker = done.front();
        done.pop();

        try {
          SmartUnlock unlock(this);
          worker->done();
        } CATCH_ERROR;
      }
    }


    // From ThreadPool
    void stop() {
      ThreadPool::stop();
      broadcast(); // Tell all threads to exit
    }


    void run() {
      SmartLock lock(this);
      Thread &thread = Thread::current();

      while (!thread.shouldShutdown()) {
        while (!ready.empty() && !thread.shouldShutdown()) {
          SmartPointer<AsyncWorker> worker = ready.front();
          ready.pop();

          try {
            SmartUnlock unlock(this);
            worker->run();
          } CATCH_ERROR;

          done.push(worker);
        }

        timedWait(0.25);
      }
    }
  };
}
