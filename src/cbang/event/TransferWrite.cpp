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

#include "TransferWrite.h"

#include <cbang/log/Logger.h>

using namespace cb::Event;
using namespace cb;


TransferWrite::TransferWrite(cb_t cb, const Buffer &buffer) :
  Transfer(cb, buffer.getLength()), buffer(buffer) {checkFinished();}


int TransferWrite::transfer(Transport &transport) {
  int ret = transport.write(buffer, buffer.getLength());
  LOG_DEBUG(4, CBANG_FUNC << "() ret=" << ret);

  if (ret < 0) finished = true;
  else {
    checkFinished();
    progress.event(ret);
  }

  return ret;
}


void TransferWrite::checkFinished() {
  if (!buffer.getLength()) finished = true;
}
