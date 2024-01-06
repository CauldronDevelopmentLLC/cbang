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

#include "TransferWrite.h"

#include <cbang/Catch.h>
#include <cbang/openssl/SSL.h>
#include <cbang/log/Logger.h>

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#include <event2/util.h>
#include <event2/buffer.h>

using namespace cb::Event;
using namespace cb;


TransferWrite::TransferWrite(int fd, const SmartPointer<SSL> &ssl, cb_t cb,
                             const Buffer &buffer) :
  Transfer(fd, ssl, cb, buffer.getLength()), buffer(buffer) {checkFinished();}


int TransferWrite::transfer() {
  int bytes = 0;

  while (true) {
    int ret = write(buffer, buffer.getLength());
    LOG_DEBUG(4, CBANG_FUNC << "() ret=" << ret);

    if (!bytes && ret < 0) finished = true;
    else checkFinished();

    if (finished || ret <= 0)
      return bytes ? bytes :
        ((success || wantsRead() || wantsWrite()) ? ret : -1);

    bytes += ret;
  }
}


int TransferWrite::write(Buffer &buffer, unsigned length) {
  if (!length) return 0;

#ifdef HAVE_OPENSSL
  if (ssl.isSet()) {
    try {
      iovec space;
      buffer.peek(length, space);
      if (!space.iov_len) return -1; // No data

      int ret = ssl->write((char *)space.iov_base, space.iov_len);
      if (0 < ret) buffer.drain(ret);

      return ret;

    } catch (const SSLException &e) {
      LOG_DEBUG(4, e.getMessage());
    }
    return -1;
  }
#endif // HAVE_OPENSSL

  return buffer.write(fd, length);
}


void TransferWrite::checkFinished() {
  if (!buffer.getLength()) finished = success = true;
}
