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

#include "FD.h"
#include "FDPool.h"
#include "TransferRead.h"
#include "TransferWrite.h"

#include <cbang/Catch.h>
#include <cbang/time/Time.h>
#include <cbang/log/Logger.h>

// For close()
#ifdef _WIN32
#include <cbang/net/Winsock.h>
#else
#include <unistd.h>
#endif

using namespace cb::Event;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "FD" << getFD() << ':'

namespace {
  void close_fd(int fd) {
#ifdef _WIN32
    closesocket((SOCKET)fd);
#else
    ::close(fd);
#endif
  }
}


FD::FD(Base &base, int fd, const SmartPointer<SSL> &ssl) :
  base(base), ssl(ssl) {
  if (0 <= fd) setFD(fd);
  LOG_DEBUG(4, CBANG_FUNC << "()");
}


FD::~FD () {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  if (fd != -1) {
    int fd = this->fd;
    TRY_CATCH_ERROR(base.getPool().flush(fd, [fd] () {close_fd(fd);}));
  }
}


void FD::setFD(int fd) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  if (0 <= this->fd) THROW("FD already set");
  this->fd = fd;
  if (0 <= fd) base.getPool().open(*this);
}


void FD::setReadTimeout(unsigned timeout)  {readTimeout  = timeout;}
void FD::setWriteTimeout(unsigned timeout) {writeTimeout = timeout;}


void FD::read(const SmartPointer<Transfer> transfer) {
  LOG_DEBUG(4, CBANG_FUNC << "() length=" << transfer->getLength());
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


void FD::canWrite(Transfer::cb_t cb) {write(new Transfer(fd, ssl, cb));}


void FD::close() {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  if (fd != -1) {
    auto onClose = this->onClose;
    int fd = this->fd;
    this->fd = -1;

    auto cb =
      [fd, onClose] () {
        close_fd(fd);
        if (onClose) onClose();
      };

    base.getPool().flush(fd, cb);
  }
}
