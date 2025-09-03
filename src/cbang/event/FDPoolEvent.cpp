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

#include "FDPoolEvent.h"
#include "Event.h"

#include <cbang/Catch.h>
#include <cbang/net/Socket.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


bool FDPoolEvent::FDQueue::wantsRead() const {
  return !empty() && front()->wantsRead();
}


bool FDPoolEvent::FDQueue::wantsWrite() const {
  return !empty() && front()->wantsWrite();
}


void FDPoolEvent::FDQueue::add(const cb::SmartPointer<Transfer> &t) {
  if (closed) TRY_CATCH_ERROR(t->complete());
  else {
    if (empty()) last = Time::now();
    push_back(t);
    popFinished();
  }
}


void FDPoolEvent::FDQueue::transfer() {
  if (!fd || empty()) return;

  if (newTransfer) {
    newTransfer = false;
    fd->progressStart(read, front()->getLength(), Time::now());
  }

  int ret = front()->transfer();
  last = Time::now();

  if (ret < 0) close();
  else {
    if (pool.getStats().isSet())
      pool.getStats()->event(read ? "read" : "write", ret, last);

    fd->progressEvent(read, ret, last);

    if (front()->isFinished()) {
      fd->progressEnd(read, front()->getLength());
      popFinished();
    }
  }
}


void FDPoolEvent::FDQueue::transferPending() {
  while (!empty() && front()->isPending()) transfer();
}


void FDPoolEvent::FDQueue::close() {
  closed = true;
  while (!empty()) pop();
}


void FDPoolEvent::FDQueue::flush() {
  clear();
  closed = true;
  fd = 0;
}


uint64_t FDPoolEvent::FDQueue::getTimeout() const {
  if (empty()) return 0;
  uint64_t timeout = front()->getTimeout();
  return timeout ? last + timeout : 0;
}


void FDPoolEvent::FDQueue::timeout() {
  uint64_t timeout = getTimeout();

  if (timeout && timeout <= Time::now()) {
    LOG_DEBUG(4, "FD" << front()->getFD() << (read ? ":Read" : ":Write")
              << " timedout");
    close();
  }
}


void FDPoolEvent::FDQueue::popFinished() {
  while (!empty() && front()->isFinished()) pop();
}


void FDPoolEvent::FDQueue::pop() {
  auto t = front();
  pop_front();
  newTransfer = true;
  TRY_CATCH_ERROR(t->complete());
}


/******************************************************************************/
FDPoolEvent::FDRec::FDRec(FDPoolEvent &pool, FD *fd) :
  fd(fd), readQ(pool, fd, true), writeQ(pool, fd, false) {
  event = pool.getBase().newEvent(
    fd->getFD(), this, &FDPoolEvent::FDRec::callback);
  timeoutEvent = pool.getBase().newEvent(this, &FDPoolEvent::FDRec::timeout);
  timeoutEvent->setPriority(pool.getEventPriority());
}


void FDPoolEvent::FDRec::read(const cb::SmartPointer<Transfer> &t) {
  readQ.add(t);
  if (readQ.size() == 1) update();
}


void FDPoolEvent::FDRec::write(const cb::SmartPointer<Transfer> &t) {
  writeQ.add(t);
  if (writeQ.size() == 1) update();
}


void FDPoolEvent::FDRec::flush() {
  readQ.flush();
  writeQ.flush();
  event->del();
  timeoutEvent->del();
  fd = 0;
}


void FDPoolEvent::FDRec::updateEvent() {
  if (!fd) return;

  unsigned events =
    (readQ.empty()  ? 0 : EF::EVENT_READ) |
    (writeQ.empty() ? 0 : EF::EVENT_WRITE);

  if (readQ.wantsWrite()) events = EF::EVENT_WRITE;
  if (writeQ.wantsRead()) events = EF::EVENT_READ;

  LOG_DEBUG(4, "FD" << fd->getFD() << ":old events=" << this->events
            << " new events=" << events);

  if (this->events == events) return;
  this->events = events;

  if (events) {
    event->renew(fd->getFD(), EF::EVENT_PERSIST | events);
    event->add();

  } else event->del();
}


void FDPoolEvent::FDRec::updateTimeout() {
  uint64_t  readTimeout =  readQ.getTimeout();
  uint64_t writeTimeout = writeQ.getTimeout();

  uint64_t timeout = readTimeout ? readTimeout : writeTimeout;
  if (writeTimeout && readTimeout && writeTimeout < readTimeout)
    timeout = writeTimeout;

  if (timeout) {
    uint64_t now = Time::now();
    timeoutEvent->add(timeout < now ? 0 : timeout - now);

  } else timeoutEvent->del();
}


void FDPoolEvent::FDRec::update() {
  readQ.transferPending();
  updateEvent();
  updateTimeout();
}


void FDPoolEvent::FDRec::callback(Event &e, int fd, unsigned events) {
  LOG_DEBUG(4, "FD" << fd << ":callback() events=" << events);

  bool read  = events & EF::EVENT_READ;
  bool write = events & EF::EVENT_WRITE;

  if (read  || (write && readQ.wantsWrite())) readQ.transfer();
  if (write || (read  && writeQ.wantsRead())) writeQ.transfer();

  update();
}


void FDPoolEvent::FDRec::timeout() {
  readQ.timeout();
  writeQ.timeout();
  updateTimeout();
}


/******************************************************************************/
FDPoolEvent::FDPoolEvent(Base &base) :
  base(base), flushEvent(base.newEvent(this, &FDPoolEvent::flushFDs)) {}


void FDPoolEvent::read(const cb::SmartPointer<Transfer> &t) {
  get(t->getFD())->second->read(t);
}


void FDPoolEvent::write(const cb::SmartPointer<Transfer> &t) {
  get(t->getFD())->second->write(t);
}


void FDPoolEvent::open(FD &_fd) {
  int fd = _fd.getFD();
  if (fd < 0) THROW("Invalid FD " << fd);
  if (!fds.insert(fds_t::value_type(fd, new FDRec(*this, &_fd))).second)
    THROW("FD already in pool");
}


void FDPoolEvent::flush(int fd) {
  auto it = get(fd);
  it->second->flush();

  // Defer FDRec deallocation, it may be executing
  flushedFDs.push_back(it->second);
  fds.erase(it);
  flushEvent->activate();

  Socket::close((socket_t)fd);
}


FDPoolEvent::fds_t::iterator FDPoolEvent::get(int fd) {
  if (fd < 0) THROW("Invalid FD " << fd);
  auto it = fds.find(fd);
  if (it == fds.end()) THROW("FD " << fd << " not found in pool");
  return it;
}


void FDPoolEvent::flushFDs() {flushedFDs.clear();}
