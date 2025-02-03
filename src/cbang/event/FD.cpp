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

#include "FD.h"
#include "FDPool.h"
#include "TransferRead.h"
#include "TransferWrite.h"

#include <cbang/Catch.h>
#include <cbang/time/Time.h>
#include <cbang/log/Logger.h>
#include <cbang/net/Socket.h>

using namespace cb::Event;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "FD" << getFD() << ':'


FD::FD(Base &base, int fd, const SmartPointer<SSL> &ssl) :
  base(base), ssl(ssl) {
  setFD(fd);
  LOG_DEBUG(4, CBANG_FUNC << "()");
}


FD::~FD () {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  if (fd != -1) {
    if (!base.isDeallocating()) TRY_CATCH_ERROR(base.getPool().flush(fd));
    else Socket::close((socket_t)fd);
  }
}


void FD::setFD(int fd) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  if (0 <= this->fd) THROW("FD already set");
  this->fd = fd < 0 ? -1 : fd;
  if (0 <= fd) base.getPool().open(*this);
}


void FD::setReadTimeout(unsigned timeout)  {readTimeout  = timeout;}
void FD::setWriteTimeout(unsigned timeout) {writeTimeout = timeout;}


void FD::progressStart(bool read, unsigned size, uint64_t time) {
  LOG_DEBUG(4, CBANG_FUNC << "() read=" << read << " size=" << size
    << " time=" << Time(time).toString());
  auto &p = read ? readProgress : writeProgress;
  p.reset();
  p.setSize(size);
  p.setStart(time);
  p.setEnd(time);
}


void FD::progressEvent(bool read, unsigned size, uint64_t time) {
  (read ? readProgress : writeProgress).event(size, time);
  (read ? readRate     : writeRate    ).event(size, time);
}


void FD::progressEnd(bool read, unsigned size) {
  auto & p = (read ? readProgress : writeProgress);
  p.setSize(size);
  p.onUpdate(true);
}


void FD::read(const SmartPointer<Transfer> transfer) {
  transfer->setTimeout(readTimeout);
  base.getPool().read(transfer);
}


void FD::read(Transfer::cb_t cb, const Buffer &buffer, unsigned length,
              const string &until) {
  read(new TransferRead(fd, ssl, cb, buffer, length, until));
}


void FD::canRead(Transfer::cb_t cb) {read(new Transfer(fd, ssl, cb));}


void FD::write(const SmartPointer<Transfer> transfer) {
  LOG_DEBUG(4, CBANG_FUNC << "() length=" << transfer->getLength());
  transfer->setTimeout(writeTimeout);
  base.getPool().write(transfer);
}


void FD::write(Transfer::cb_t cb, const Buffer &buffer) {
  write(new TransferWrite(fd, ssl, cb, buffer));
}


void FD::canWrite(Transfer::cb_t cb) {
  write(new Transfer(fd, ssl, cb));
}


void FD::close() {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  if (fd != -1) {
    int fd = this->fd;
    this->fd = -1;

    base.getPool().flush(fd);
  }
}
