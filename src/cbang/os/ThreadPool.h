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

#include "Thread.h"

#include <cbang/SmartPointer.h>

#include <vector>


namespace cb {
  /// Multiple work threads execute a single run() function.
  class ThreadPool {
    typedef std::vector<SmartPointer<Thread> > pool_t;
    pool_t pool;

  public:
    ThreadPool(unsigned size);
    virtual ~ThreadPool() {}

    virtual void start();
    virtual void stop();
    virtual void join();
    virtual void wait();

    void getStates(std::vector<Thread::state_t> &states) const;
    void getIDs(std::vector<unsigned> &ids) const;
    void getExitStatuses(std::vector<int> &statuses) const;

    virtual void cancel();
    virtual void kill(int signal);

  protected:
    /**
     * This function should be overriden by sub classes and will be called
     * concurrently by threads running in the pool.
     *
     * cb::Thread::current().shouldShutdown() should called by any long-running
     * process to check for requested shutdown.
     */
    virtual void run() = 0;

    typedef pool_t::const_iterator iterator;
    iterator begin() const {return pool.begin();}
    iterator end() const {return pool.end();}
  };
}
