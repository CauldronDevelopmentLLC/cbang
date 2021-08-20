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

#include "FDPoolEvent.h"
#include "Event.h"

#include <cbang/Catch.h>

using namespace cb::Event;


bool FDPoolEvent::FDQueue::wantsRead() const {
  return !empty() && front()->wantsRead();
}


bool FDPoolEvent::FDQueue::wantsWrite() const {
  return !empty() && front()->wantsWrite();
}


void FDPoolEvent::FDQueue::add(const SmartPointer<Transfer> &t) {
  if (closed) TRY_CATCH_ERROR(t->complete());
  else push_back(t);
}


void FDPoolEvent::FDQueue::transfer() {
  if (empty()) return;

  int ret = front()->transfer();
  if (empty()) return;

  if (ret < 0) {
    while (!empty()) {
      TRY_CATCH_ERROR(front()->complete());
      if (!empty()) pop_front();
    }

    closed = true;

  } else if (front()->isFinished()) {
    TRY_CATCH_ERROR(front()->complete());
    if (!empty()) pop_front();
  }
}


void FDPoolEvent::FDQueue::flush() {
  clear();
  closed = true;
}


/******************************************************************************/
FDPoolEvent::FDRec::FDRec(FDPoolEvent &pool, int fd) :
  pool(pool), fd(fd) {
  event = pool.getBase().newEvent(fd, this, &FDPoolEvent::FDRec::callback,
                                  EF::EVENT_NO_SELF_REF);
}


void FDPoolEvent::FDRec::read(const SmartPointer<Transfer> &t) {
  readQ.add(t);
  updateEvent();
}


void FDPoolEvent::FDRec::write(const SmartPointer<Transfer> &t) {
  writeQ.add(t);
  updateEvent();
}


void FDPoolEvent::FDRec::flush() {
  readQ.flush();
  writeQ.flush();
  event->del();
}


void FDPoolEvent::FDRec::updateEvent() {
  unsigned events =
    (readQ.empty()  ? 0 : EF::EVENT_READ) |
    (writeQ.empty() ? 0 : EF::EVENT_WRITE);

  if (this->events == events) return;
  this->events = events;

  if (events) {
    event->renew(fd, EF::EVENT_PERSIST | events);
    event->setPriority(pool.getEventPriority());
    event->add();

  } else event->del();
}


void FDPoolEvent::FDRec::callback(Event &e, int fd, unsigned events) {
  readQ.transfer();
  writeQ.transfer();
  updateEvent();
}


/******************************************************************************/
FDPoolEvent::FDPoolEvent(Base &base) : base(base) {
  releaseEvent = base.newEvent(this, &FDPoolEvent::releaseCB);
}


void FDPoolEvent::read(const SmartPointer<Transfer> &t) {
  get(t->getFD()).read(t);
}


void FDPoolEvent::write(const SmartPointer<Transfer> &t) {
  get(t->getFD()).write(t);
}


void FDPoolEvent::open(FD &_fd) {
  int fd = _fd.getFD();
  if (fd < 0) THROW("Invalid FD " << fd);
  if (!fds.insert(fds_t::value_type(fd, new FDRec(*this, fd))).second)
    THROW("FD already in pool");
}


void FDPoolEvent::flush(int fd, std::function <void ()> cb) {
  get(fd).flush();

  // Defer FDRec deallocation, it may be executing
  auto it = fds.find(fd);
  releaseFDs.push_back(it->second);
  fds.erase(it);
  releaseEvent->activate();

  cb();
}


FDPoolEvent::FDRec &FDPoolEvent::get(int fd) {
  if (fd < 0) THROW("Invalid FD " << fd);
  auto it = fds.find(fd);
  if (it == fds.end()) THROW("FD " << fd << " not found in pool");
  return *it->second;
}


void FDPoolEvent::releaseCB() {releaseFDs.clear();}
