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

#include "FD.h"
#include "FDPool.h"
#include "TransferRead.h"
#include "TransferWrite.h"

#include <cbang/Catch.h>
#include <cbang/util/SmartLock.h>
#include <cbang/util/SmartUnlock.h>
#include <cbang/time/Time.h>
#include <cbang/log/Logger.h>

#include <limits>

#include <unistd.h> // For close()

using namespace cb::Event;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX << "FD" << getFD() << ':'


FD::FD(Base &base, int fd, const SmartPointer<SSL> &ssl) :
  base(base), fd(fd), ssl(ssl),
  completeEvent(base.newEvent(this, &FD::complete)),
  timeoutEvent(base.newEvent(this, &FD::timeout)) {

  LOG_DEBUG(4, CBANG_FUNC << "() fd=" << fd);

  completeEvent->setSelfReferencing(false);
  timeoutEvent->setSelfReferencing(false);
}


FD::~FD () {
  completeEvent->del();
  timeoutEvent->del();
  poolRemove();
}


void FD::setFD(int fd) {
  SmartLock lock(this);
  if (inPool) THROW("Cannot modify FD while active");
  this->fd = fd;
  if (transport.isSet()) transport.release();
  poolAdd();
}


void FD::setPriority(int priority) {
  completeEvent->setPriority(priority);
  timeoutEvent->setPriority(priority);
}


int FD::getPriority() const {return completeEvent->getPriority();}


void FD::setReadTimeout(unsigned timeout) {
  SmartLock lock(this);
  readTimeout = timeout;
  updateTimeout();
}


void FD::setWriteTimeout(unsigned timeout) {
  SmartLock lock(this);
  writeTimeout = timeout;
  updateTimeout();
}


Progress FD::getReadProgress() const {
  SmartLock lock(this);
  return readQ.size() ? readQ.front()->getProgress() : Progress();
}


Progress FD::getWriteProgress() const {
  SmartLock lock(this);
  return writeQ.size() ? writeQ.front()->getProgress() : Progress();
}


void FD::read(const SmartPointer<Transfer> transfer) {
  LOG_DEBUG(4, CBANG_FUNC << "() length=" << transfer->getLength());

  SmartLock lock(this);
  if (readClosed) THROW("Read closed");

  if (transfer->isFinished()) {
    completeQ.push(transfer);
    return complete();

  } else readQ.push(transfer);

  if (readQ.size() == 1) {
    lastRead = Time::now(); // Timeout starts now
    update();
  }
}


void FD::read(Transfer::cb_t cb, const Buffer &buffer, unsigned length,
              const string &until) {
  read(new TransferRead(cb, buffer, length, until));
}


void FD::canRead(Transfer::cb_t cb) {read(new Transfer(cb));}


void FD::write(const SmartPointer<Transfer> transfer) {
  LOG_DEBUG(4, CBANG_FUNC << "() length=" << transfer->getLength());

  SmartLock lock(this);
  if (writeClosed) THROW("Write closed");

  if (transfer->isFinished()) {
    completeQ.push(transfer);
    return complete();

  } else writeQ.push(transfer);

  if (writeQ.size() == 1) {
    lastWrite = Time::now(); // Timeout starts now
    update();
  }
}


void FD::write(Transfer::cb_t cb, const Buffer &buffer) {
  write(new TransferWrite(cb, buffer));
}


void FD::canWrite(Transfer::cb_t cb) {write(new Transfer(cb));}


void FD::close() {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  SmartLock lock(this);

  readClosed = writeClosed = true;
  poolRemove(); // Must be before ::close(fd)

  timeoutEvent->del();
  completeEvent->activate();

  if (fd != -1) {
    ::close(fd);
    fd = -1;
  }
}


void FD::transfer(unsigned events) {
  SmartLock lock(this);

  LOG_DEBUG(4, CBANG_FUNC << '(' << eventsToString(events) << ')'
            << " events=" << eventsToString(getEvents())
            << " readWantsWrite=" << readWantsWrite
            << " writeWantsRead=" << writeWantsRead
            << " readQ=" << readQ.size() << " writeQ=" << writeQ.size()
            << " readClosed=" << readClosed << " writeClosed=" << writeClosed);

  read(events);
  write(events);

  update();
}


void FD::flush(queue_t &q, bool success) {
  while (!q.empty()) {
    auto transfer = q.front();
    q.pop();
    TRY_CATCH_ERROR(transfer->complete(success));
  }
}


int FD::transfer(queue_t &q) {
  const auto &transfer = q.front();

  int ret = transfer->transfer(*transport);

  if (ret < 0 || transfer->isFinished()) {
    completeQ.push(transfer);
    q.pop();
    completeEvent->activate();
  }

  return ret;
}


void FD::read(unsigned events) {
  if (!readQ.empty() && !readClosed &&
      ((events & READ_EVENT) || ((events & WRITE_EVENT) && readWantsWrite))) {
    try {
      lastRead = Time::now();
      int ret = transfer(readQ);
      if (0 <= ret) {
        readWantsWrite = transport->wantsWrite();
        return;
      }
    } CATCH_DEBUG(4);

    readClosed = true;
    completeEvent->activate();
  }
}


void FD::write(unsigned events) {
  if (!writeQ.empty() && !writeClosed &&
      ((events & WRITE_EVENT) || ((events & READ_EVENT) && writeWantsRead))) {
    try {
      lastWrite = Time::now();
      int ret = transfer(writeQ);
      if (0 <= ret) {
        writeWantsRead = transport->wantsRead();
        return;
      }
    } CATCH_DEBUG(4);

    writeClosed = true;
    completeEvent->activate();
  }
}


bool FD::readTimeoutValid() const {
  return !readQ.empty() && readTimeout && lastRead;
}


bool FD::writeTimeoutValid() const {
  return !writeQ.empty() && writeTimeout && lastWrite;
}


string FD::eventsToString(unsigned events) {
  vector<string> v;

  if (events & READ_EVENT)    v.push_back("READ");
  if (events & WRITE_EVENT)   v.push_back("WRITE");
  if (events & CLOSE_EVENT)   v.push_back("CLOSE");
  if (events & TIMEOUT_EVENT) v.push_back("TIMEOUT");

  return String::join(v, "|");
}


unsigned FD::getEvents() const {
  unsigned events =
    (readQ.empty() ? 0 : READ_EVENT) | (writeQ.empty() ? 0 : WRITE_EVENT);

  if (transport->wantsRead())  events = READ_EVENT;
  if (transport->wantsWrite()) events = WRITE_EVENT;

  if (readClosed)  events &= ~READ_EVENT;
  if (writeClosed) events &= ~WRITE_EVENT;

  return events;
}


void FD::updateEvents() {
  if (!inPool) return;

  if (lastEvents != getEvents()) {
    LOG_DEBUG(4, CBANG_FUNC << "() " << eventsToString(lastEvents) << "->"
              << eventsToString(getEvents()));
    base.getPool().update(*this, getEvents());
    lastEvents = getEvents();
  }
}


void FD::updateTimeout() {
  uint64_t t = numeric_limits<uint64_t>::max();

  if (readTimeoutValid())  t = min(t, lastRead  + readTimeout);
  if (writeTimeoutValid()) t = min(t, lastWrite + writeTimeout);

  if (t == numeric_limits<uint64_t>::max()) timeoutEvent->del();
  else timeoutEvent->add(t - Time::now());
}


void FD::update() {
  updateEvents();
  updateTimeout();
}


void FD::complete() {
  {
    SmartLock lock(this);
    flush(completeQ, true);

    if (readClosed)  flush(readQ,  false);
    if (writeClosed) flush(writeQ, false);

    if (!readClosed || !writeClosed) return;

    completeEvent->del();
    timeoutEvent->del();
    poolRemove();
  }

  if (onComplete) onComplete();
}


void FD::timeout() {
  uint64_t now = Time::now();
  SmartLock lock(this);

  if (readTimeoutValid() && lastRead + readTimeout <= now) {
    LOG_DEBUG(4, CBANG_FUNC << "() read timedout");
    flush(readQ, false);
    readTimedout = true;
  }

  if (writeTimeoutValid() && lastWrite + writeTimeout <= now) {
    LOG_DEBUG(4, CBANG_FUNC << "() write timeout");
    flush(writeQ, false);
    writeTimedout = true;
  }

  update();
}


void FD::poolRemove() {
  if (!inPool) return;
  base.getPool().remove(*this);
  inPool = false;
}


void FD::poolAdd() {
  if (inPool) return;

  if (fd == -1) THROW("FD not set");

  if (transport.isNull()) {
    if (ssl.isSet()) transport = Transport::make(ssl);
    else transport = Transport::make(fd);
  }

  base.getPool().add(*this);
  inPool = true;
}
