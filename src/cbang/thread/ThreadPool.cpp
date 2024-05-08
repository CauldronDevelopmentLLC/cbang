/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "ThreadPool.h"

using namespace cb;
using namespace std;


ThreadPool::ThreadPool(unsigned size) {
  for (unsigned i = 0; i < size; i++)
    pool.push_back(new ThreadFunc([this] {run();}));
}


void ThreadPool::start() {for (auto &t: *this) t->start();}
void ThreadPool::stop()  {for (auto &t: *this) t->stop();}


void ThreadPool::join() {
  for (auto &t: *this) t->stop();
  for (auto &t: *this) t->wait();
}


void ThreadPool::wait() {for (auto &t: *this) t->wait();}


void ThreadPool::getStates(vector<Thread::state_t> &states) const {
  for (auto &t: *this) states.push_back(t->getState());
}


void ThreadPool::getIDs(vector<unsigned> &ids) const {
  for (auto &t: *this) ids.push_back(t->getID());
}


void ThreadPool::getExitStatuses(vector<int> &statuses) const {
  for (auto &t: *this) statuses.push_back(t->getExitStatus());
}


void ThreadPool::cancel()         {for (auto &t: *this) t->cancel();}
void ThreadPool::kill(int signal) {for (auto &t: *this) t->kill(signal);}
