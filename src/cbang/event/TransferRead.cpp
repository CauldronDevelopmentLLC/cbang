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

#include "TransferRead.h"

#include <cbang/String.h>
#include <cbang/openssl/SSL.h>
#include <cbang/log/Logger.h>

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#include <event2/util.h>

using namespace cb::Event;
using namespace std;


TransferRead::TransferRead(int fd, const SmartPointer<SSL> &ssl, cb_t cb,
                           const Buffer &buffer, unsigned length,
                           const string &until) :
  Transfer(fd, ssl, cb, length), buffer(buffer), until(until) {checkFinished();}



int TransferRead::transfer() {
  int ret = read(buffer, length - buffer.getLength());
  LOG_DEBUG(4, CBANG_FUNC << "() ret=" << ret
            << " length=" << buffer.getLength());
  LOG_DEBUG(5, String::hexdump(buffer.toString()));

  if (ret < 0) finished = true;
  else checkFinished();

  return ret;
}


int TransferRead::read(Buffer &buffer, unsigned length) {
  if (!length) return 0;

#ifdef HAVE_OPENSSL
  if (ssl.isSet()) {
    iovec space;
    buffer.reserve(length, space);
    if (!space.iov_len) return -1;

    unsigned bytes = min(length, (unsigned)space.iov_len);
    int ret = ssl->read((char *)space.iov_base, bytes);

    if (0 < ret) {
#ifdef VALGRIND_MAKE_MEM_DEFINED
      (void)VALGRIND_MAKE_MEM_DEFINED(space.iov_base, ret);
#endif

      space.iov_len = ret;
      buffer.commit(space);
    }

    return ret;
  }
#endif // HAVE_OPENSSL

  int ret = buffer.read(fd, length);
  return ret <= 0 ? -1 : ret;
}


void TransferRead::checkFinished() {
  if (finished) return;

  unsigned bytesRead = buffer.getLength();
  if (length <= bytesRead || (!until.empty() && buffer.indexOf(until) != -1)) {
    finished = success = true;
    length = bytesRead;
  }
}
