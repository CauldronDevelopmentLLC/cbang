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

#include "Event.h"

#include <cbang/util/SmartLock.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SysError.h>

#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


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


  const char *epollOpString(int op) {
    switch (op) {
    case EPOLL_CTL_MOD: return "mod";
    case EPOLL_CTL_DEL: return "del";
    case EPOLL_CTL_ADD: return "add";
    default: return "???";
    }
  }
}



/******************************************************************************/
bool FDPoolEPoll::FDQueue::wantsRead() const {
  return !empty() && front()->wantsRead();
}


bool FDPoolEPoll::FDQueue::wantsWrite() const {
  return !empty() && front()->wantsWrite();
}


uint64_t FDPoolEPoll::FDQueue::getTimeout() const {
  return empty() ? 0 : front()->getTimeout();
}


uint64_t FDPoolEPoll::FDQueue::getNextTimeout() const {
  return (!last || empty()) ? 0 : (getTimeout() + last);
}


void FDPoolEPoll::FDQueue::updateTimeout(bool wasActive, bool nowActive) {
  if (!nowActive || closed) last = 0;
  else if (!wasActive) {
    last = Time::now();

    if (getTimeout())
      fdr.getPool().queueTimeout(getNextTimeout(), read, fdr.getFD());
  }
}


void FDPoolEPoll::FDQueue::timeout(uint64_t now) {
  if (!getTimeout() || !last || closed) return;

  if (getNextTimeout() < now) {
    LOG_DEBUG(4, (read ? "Read" : "Write")
              << " timedout on fd=" << fdr.getFD());
    close();
    timedout = true;

  } else fdr.getPool().queueTimeout(getNextTimeout(), read, fdr.getFD());
}


void FDPoolEPoll::FDQueue::transfer() {
  if (closed || empty()) return;

  auto &pool = fdr.getPool();

  if (newTransfer) {
    newTransfer = false;
    cmd_t cmd = read ? CMD_READ_SIZE : CMD_WRITE_SIZE;
    pool.queueProgress(cmd, fdr.getFD(), Time::now(), front()->getLength());
  }

  int ret = front()->transfer();

  if (ret < 0) close();
  else {
    last = Time::now();

    cmd_t cmd = read ? CMD_READ_PROGRESS : CMD_WRITE_PROGRESS;
    pool.queueProgress(cmd, fdr.getFD(), last, ret);

    if (front()->isFinished()) {
      cmd = read ? CMD_READ_FINISHED : CMD_WRITE_FINISHED;
      pool.queueProgress(cmd, fdr.getFD(), last, front()->getLength());
      pool.queueComplete(front());
      pop();
    }
  }
}


void FDPoolEPoll::FDQueue::transferPending() {
  while (!empty() && front()->isPending()) transfer();
}


void FDPoolEPoll::FDQueue::flush() {
  while (!empty()) pop();
  closed = timedout = false;
  last = 0;
}


void FDPoolEPoll::FDQueue::add(const SmartPointer<Transfer> &tran) {
  if (closed) fdr.getPool().queueComplete(tran);
  else push(tran);
}


void FDPoolEPoll::FDQueue::close() {
  closed = true;

  while (!empty()) {
    fdr.getPool().queueComplete(front());
    pop();
  }
}


void FDPoolEPoll::FDQueue::pop() {
  queue<SmartPointer<Transfer> >::pop();
  newTransfer = true;
}



/******************************************************************************/
FDPoolEPoll::FDRec::FDRec(FDPoolEPoll &pool, int fd) :
  pool(pool), fd(fd), readQ(*this, true), writeQ(*this, false) {}


void FDPoolEPoll::FDRec::timeout(uint64_t now) {
  readQ.timeout(now);
  writeQ.timeout(now);
}


unsigned FDPoolEPoll::FDRec::getEvents() const {
  if (writeQ.wantsRead()) return FD::READ_EVENT;
  if (readQ.wantsWrite()) return FD::WRITE_EVENT;

  return
    ((readQ.empty()  || readQ.isClosed())  ? 0 : FD::READ_EVENT) |
    ((writeQ.empty() || writeQ.isClosed()) ? 0 : FD::WRITE_EVENT);
}


int FDPoolEPoll::FDRec::getStatus() const {
  return getEvents() |
    (readQ.isClosed()    ? STATUS_READ_CLOSED    : 0) |
    (writeQ.isClosed()   ? STATUS_WRITE_CLOSED   : 0) |
    (readQ.isTimedout()  ? STATUS_READ_TIMEDOUT  : 0) |
    (writeQ.isTimedout() ? STATUS_WRITE_TIMEDOUT : 0);
}


void FDPoolEPoll::FDRec::transfer(unsigned events) {
  if (((events & FD::WRITE_EVENT) && readQ.wantsWrite()) ||
      ((events & FD::READ_EVENT)  && !writeQ.wantsRead()))
    readQ.transfer();

  if (((events & FD::READ_EVENT)  && writeQ.wantsRead()) ||
      ((events & FD::WRITE_EVENT) && !readQ.wantsWrite()))
    writeQ.transfer();

  update();
}


void FDPoolEPoll::FDRec::flush() {
  readQ.flush();
  writeQ.flush();
  pool.queueFlushed(fd);
}


void FDPoolEPoll::FDRec::process(cmd_t cmd,
                                 const SmartPointer<Transfer> &tran) {
  if ((cmd == CMD_READ || cmd == CMD_WRITE) && tran->isFinished())
    return pool.queueComplete(tran);

  switch (cmd) {
  case CMD_READ:  readQ.add(tran);  break;
  case CMD_WRITE: writeQ.add(tran); break;
  case CMD_FLUSH: flush();          break;
  default: LOG_ERROR("Invalid command");
  }

  update();
}


void FDPoolEPoll::FDRec::update() {
  readQ.transferPending();

  unsigned newEvents = getEvents();
  if (events == newEvents) return;

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = fd_to_epoll_events(newEvents);
  ev.data.fd = fd;
  int op = events ? (newEvents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL) : EPOLL_CTL_ADD;

  if (epoll_ctl(pool.getFD(), op, fd, &ev))
    if (op != EPOLL_CTL_DEL)
      LOG_ERROR("epoll_ctl(" << epollOpString(op) << ") failed for fd " << fd
                << ": " << SysError());

  // Update timeouts
  readQ.updateTimeout(events & FD::READ_EVENT, newEvents & FD::READ_EVENT);
  writeQ.updateTimeout(events & FD::WRITE_EVENT, newEvents & FD::WRITE_EVENT);

  // Update events
  events = newEvents;
}



/******************************************************************************/
FDPoolEPoll::FDPoolEPoll(Base &base) :
  event(base.newEvent(this, &FDPoolEPoll::processResults)) {

  event->setSelfReferencing(false);

  fd = epoll_create1(EPOLL_CLOEXEC);
  if (!fd) THROW("Failed to create epoll");
  start();
}


FDPoolEPoll::~FDPoolEPoll() {
  join();
  if (fd != -1) close(fd);
}


void FDPoolEPoll::setEventPriority(int priority) {event->setPriority(priority);}
int FDPoolEPoll::getEventPriority() const {return event->getPriority();}


void FDPoolEPoll::read(const SmartPointer<Transfer> &t) {
  if (t.isNull()) THROW("Transfer cannot be null");
  queueCommand(CMD_READ, t->getFD(), t);
}


void FDPoolEPoll::write(const SmartPointer<Transfer> &t) {
  if (t.isNull()) THROW("Transfer cannot be null");
  queueCommand(CMD_WRITE, t->getFD(), t);
}


void FDPoolEPoll::open(FD &fd) {
  if (fd.getFD() < 0) THROW("Invalid fd " << fd.getFD());
  if (!fds.insert(fds_t::value_type(fd.getFD(), &fd)).second)
    THROW("FD already in pool");
}


void FDPoolEPoll::flush(int fd, std::function <void ()> cb) {
  if (fd < 0) THROW("Invalid fd " << fd);
  if (flushing.find(fd) != flushing.end())
    THROW("FD " << fd << " already flushing");
  flushing[fd] = cb;
  queueCommand(CMD_FLUSH, fd, 0);
}


void FDPoolEPoll::queueTimeout(uint64_t time, bool read, int fd) {
  timeoutQ.push({time, read, fd});
}


void FDPoolEPoll::queueComplete(const SmartPointer<Transfer> &t) {
  results.push({CMD_COMPLETE, t->getFD(), t, 0, 0});
  event->activate();
}


void FDPoolEPoll::queueFlushed(int fd) {
  results.push({CMD_FLUSHED, fd, 0, 0, 0});
  event->activate();
}


void FDPoolEPoll::queueProgress(cmd_t cmd, int fd, uint64_t time, int value) {
  results.push({cmd, fd, 0, time, value});
  event->activate();
}


void FDPoolEPoll::queueStatus(int fd, int status) {
  results.push({CMD_STATUS, fd, 0, 0, status});
  event->activate();
}


void FDPoolEPoll::queueCommand(cmd_t cmd, int fd,
                               const SmartPointer<Transfer> &tran) {
  cmds.push({cmd, fd, tran});
}


FDPoolEPoll::FDRec &FDPoolEPoll::getFD(int fd) {
  auto it = pool.find(fd);
  if (it != pool.end()) return *it->second;
  auto result = pool.insert(pool_t::value_type(fd, new FDRec(*this, fd)));
  return *result.first->second;
}


void FDPoolEPoll::processResults() {
  while (!results.empty()) {
    auto &cmd = results.top();
    LOG_DEBUG(5, CBANG_FUNC << "() fd=" << cmd.fd << " cmd=" << cmd.cmd);

    auto it = fds.find(cmd.fd);
    if (it == fds.end()) {
      results.pop();
      continue;
    }

    FD &fd = *it->second;
    auto it2 = flushing.find(cmd.fd);
    if (it2 != flushing.end() && cmd.cmd != CMD_FLUSHED) {
      results.pop();
      continue;
    }

    switch (cmd.cmd) {
    case CMD_FLUSHED:
      if (it2->second) it2->second(); // Call callback
      flushing.erase(it2);
      fds.erase(cmd.fd);
      break;

    case CMD_COMPLETE: TRY_CATCH_ERROR(cmd.tran->complete()); break;

    case CMD_READ_PROGRESS:
      readRate.event(cmd.value, cmd.time);
      fd.getReadProgress().event(cmd.value, cmd.time);
      break;

    case CMD_WRITE_PROGRESS:
      writeRate.event(cmd.value, cmd.time);
      fd.getWriteProgress().event(cmd.value, cmd.time);
      break;

    case CMD_READ_SIZE:
      fd.getReadProgress().reset();
      fd.getReadProgress().setSize(cmd.value);
      fd.getReadProgress().setStart(cmd.time);
      fd.getReadProgress().setEnd(cmd.time);
      break;

    case CMD_WRITE_SIZE:
      fd.getWriteProgress().reset();
      fd.getWriteProgress().setSize(cmd.value);
      fd.getWriteProgress().setStart(cmd.time);
      fd.getWriteProgress().setEnd(cmd.time);
      break;

    case CMD_READ_FINISHED:  fd.getReadProgress().setSize(cmd.value);  break;
    case CMD_WRITE_FINISHED: fd.getWriteProgress().setSize(cmd.value); break;

    case CMD_STATUS: fd.setStatus(cmd.value); break;

    default: LOG_ERROR("Invalid results command");
    }

    results.pop();
  }
}


void FDPoolEPoll::run() {
  epoll_event records[1024];

  while (!shouldShutdown()) {
    int count = epoll_wait(this->fd, records, 1024, 100);
    if (count == -1) {
      if (errno != EINTR) {
        LOG_ERROR("epoll_wait() failed");
        break;
      }
    }

    set<int> changed;

    for (int i = 0; i < count; i++)
      try {
        unsigned events = epoll_to_fd_events(records[i].events);
        int fd = records[i].data.fd;
        getFD(fd).transfer(events);
        changed.insert(fd);
      } CATCH_ERROR;

    // Process pending commands
    while (!cmds.empty()) {
      auto &cmd = cmds.top();
      getFD(cmd.fd).process(cmd.cmd, cmd.tran);
      changed.insert(cmd.fd);
      cmds.pop();
    }

    // Process timeouts
    uint64_t now = Time::now();
    while (!timeoutQ.empty() && timeoutQ.top().time < now) {
      int fd = timeoutQ.top().fd;
      getFD(fd).timeout(now);
      changed.insert(fd);
      timeoutQ.pop();
    }

    // Queue status changes
    for (auto it = changed.begin(); it != changed.end(); it++)
      queueStatus(*it, getFD(*it).getStatus());
  }
}

#endif // HAVE_EPOLL
