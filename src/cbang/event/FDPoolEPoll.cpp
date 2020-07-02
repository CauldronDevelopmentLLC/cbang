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

#include "FDPoolEPoll.h"

#ifdef HAVE_EPOLL

#include "Base.h"

#include <cbang/util/SmartLock.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SysError.h>

#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

using namespace cb::Event;
using namespace cb;


namespace {
  uint32_t fd_to_epoll_events(unsigned events) {
    return
      ((FD::CLOSE_EVENT & events) ? EPOLLRDHUP : 0) |
      ((FD::READ_EVENT  & events) ? EPOLLIN    : 0) |
      ((FD::WRITE_EVENT & events) ? EPOLLOUT   : 0);
  }


  unsigned epoll_to_fd_events(uint32_t events) {
    unsigned e = 0;

    if (events & (EPOLLERR | EPOLLHUP)) e = FD::READ_EVENT | FD::WRITE_EVENT;
    else {
      if (events & EPOLLIN)    e |= FD::READ_EVENT;
      if (events & EPOLLOUT)   e |= FD::WRITE_EVENT;
      if (events & EPOLLRDHUP) e |= FD::CLOSE_EVENT;
    }

    return e;
  }


  bool wrapped_less(uint32_t a, uint32_t b) {
    return b - a < ((uint32_t)1 << 31);
  }
}


FDPoolEPoll::FDPoolEPoll(Base &base) :
  event(base.newEvent(this, &FDPoolEPoll::doRemove)) {

  fd = epoll_create1(EPOLL_CLOEXEC);
  if (!fd) THROW("Failed to create epoll");
  start();
}


FDPoolEPoll::~FDPoolEPoll() {
  join();
  if (fd != -1) close(fd);

  destructing = true;
  pool.clear();
  removeQ.clear();
}


void FDPoolEPoll::add(FD &fd) {
  if (destructing) return;

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = 0;
  ev.data.ptr = &fd;

  SmartLock lock(this);

  if (!pool.insert(pool_t::value_type(fd.getFD(), &fd)).second)
    THROW("FD " << fd.getFD() << " already in pool");

  if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd.getFD(), &ev))
    THROW("Failed to add FD: " << SysError());
}


void FDPoolEPoll::update(FD &fd, unsigned events) {
  if (destructing) return;

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = fd_to_epoll_events(events);
  ev.data.ptr = &fd;

  if (epoll_ctl(this->fd, EPOLL_CTL_MOD, fd.getFD(), &ev))
    THROW("Failed to modify fd: " << SysError());
}


void FDPoolEPoll::remove(FD &fd) {
  if (destructing) return;

  // May already be removed if FD was closed
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd.getFD(), 0);

  SmartLock lock(this);
  sync = true;
  removeQ.push_back(remove_t(count, &fd));
  pool.erase(fd.getFD());
}


void FDPoolEPoll::doRemove() {
  SmartLock lock(this);

  while (!removeQ.empty() && wrapped_less(removeQ.front().first, count))
    removeQ.pop_front();
}


void FDPoolEPoll::run() {
  epoll_event records[1024];

  while (!shouldShutdown()) {
    {
      SmartLock lock(this);
      if (sync) {
        sync = false;
        event->activate();
        count++;
      }
    }

    int count = epoll_wait(this->fd, records, 1024, 250);
    if (count == -1) {
      if (errno == EINTR) continue;
      LOG_ERROR("epoll_wait() failed");
      break;
    }

    for (int i = 0; i < count; i++)
      try {
        unsigned events = epoll_to_fd_events(records[i].events);
        ((FD *)records[i].data.ptr)->transfer(events);
      } CATCH_ERROR;
  }
}

#endif // HAVE_EPOLL
