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

#include "TransportSSL.h"
#ifdef HAVE_OPENSSL

#include <cbang/openssl/SSL.h>
#include <cbang/log/Logger.h>

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#include <event2/util.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


TransportSSL::TransportSSL(const SmartPointer<SSL> &ssl) : ssl(ssl) {}


int TransportSSL::read(Buffer &buffer, unsigned length) {
  if (!length) return 0;

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


int TransportSSL::write(Buffer &buffer, unsigned length) {
  if (!length) return 0;

  iovec space;
  buffer.peek(length, space);
  if (!space.iov_len) return -1; // No data

  int ret = ssl->write((char *)space.iov_base, space.iov_len);
  if (0 < ret) buffer.drain(ret);

  return ret;
}

#endif // HAVE_OPENSSL
