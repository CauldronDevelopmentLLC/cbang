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

#include "FDPoolEvent.h"
#include "Event.h"

#include <cbang/Catch.h>

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
  if (empty()) return;

  int ret = front()->transfer();
  last = Time::now();

  if (ret < 0) close();
  else popFinished();
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
  TRY_CATCH_ERROR(t->complete());
}


/******************************************************************************/
FDPoolEvent::FDRec::FDRec(FDPoolEvent &pool, int fd) :
  pool(pool), fd(fd), readQ(true), writeQ(false) {
  event = pool.getBase().newEvent(fd, this, &FDPoolEvent::FDRec::callback,
                                  EF::EVENT_NO_SELF_REF);
  timeoutEvent = pool.getBase().newEvent(this, &FDPoolEvent::FDRec::timeout,
                                         EF::EVENT_NO_SELF_REF);
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
}


void FDPoolEvent::FDRec::updateEvent() {
  unsigned events =
    (readQ.empty()  ? 0 : EF::EVENT_READ) |
    (writeQ.empty() ? 0 : EF::EVENT_WRITE);

  if (readQ.wantsWrite()) events = EF::EVENT_WRITE;
  if (writeQ.wantsRead()) events = EF::EVENT_READ;

  LOG_DEBUG(4, "FD" << fd << ":old events=" << this->events
            << " new events=" << events);

  if (this->events == events) return;
  this->events = events;

  if (events) {
    event->renew(fd, EF::EVENT_PERSIST | events);
    event->setPriority(pool.getEventPriority());
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
FDPoolEvent::FDPoolEvent(Base &base) : base(base) {}


void FDPoolEvent::read(const cb::SmartPointer<Transfer> &t) {
  get(t->getFD()).read(t);
}


void FDPoolEvent::write(const cb::SmartPointer<Transfer> &t) {
  get(t->getFD()).write(t);
}


void FDPoolEvent::open(FD &_fd) {
  int fd = _fd.getFD();
  if (fd < 0) THROW("Invalid FD " << fd);
  if (!fds.insert(fds_t::value_type(fd, new FDRec(*this, fd))).second)
    THROW("FD already in pool");
}


void FDPoolEvent::flush(int fd, function <void ()> cb) {
  get(fd).flush();

  // Defer FDRec deallocation, it may be executing
  auto it    = fds.find(fd);
  auto fdPtr = it->second;
  base.newEvent([fdPtr] () {}, 0)->add();
  fds.erase(it);

  cb();
}


FDPoolEvent::FDRec &FDPoolEvent::get(int fd) {
  if (fd < 0) THROW("Invalid FD " << fd);
  auto it = fds.find(fd);
  if (it == fds.end()) THROW("FD " << fd << " not found in pool");
  return *it->second;
}
