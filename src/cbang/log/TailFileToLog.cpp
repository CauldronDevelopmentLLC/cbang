/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#include "TailFileToLog.h"

#include <cbang/String.h>
#include <cbang/time/Timer.h>
#include <cbang/os/SystemUtilities.h>

#include <string.h>

using namespace std;
using namespace cb;


void TailFileToLog::run() {
  Timer timer;

  while (!shouldShutdown()) {
    if (stream.isNull()) {
      if (SystemUtilities::exists(filename))
        stream = SystemUtilities::open(filename, ios::in);

    } else {
      while (!stream->fail() && !shouldShutdown()) {
        // Try to read some data
        stream->read(buffer + fill, bufferSize - fill);
        fill += stream->gcount();

        while (fill) {
          // Find end of line
          buffer[fill] = 0; // Terminate buffer
          char *eol = strchr(buffer, '\n');

          if (eol) {
            // Terminate line
            char *ptr = eol;
            *ptr-- = 0;
            if (*ptr == '\r') *ptr = 0;

            // Log the line
            log();

            // Update buffer
            fill -= (eol - buffer) + 1;
            if (fill) {
              memmove(buffer, eol + 1, fill);
              continue;
            }

          } else { // EOL not found
            if (fill == bufferSize) {
              // Buffer is full so just log it as is
              log();
              fill = 0;
            }

            break;
          }
        }

        // Ignore end of stream but not bad/closed stream
        if (stream->eof() && !stream->bad()) {
          stream->clear();
          break;
        }
      }
    }

    timer.throttle(0.25);
  }
}


void TailFileToLog::log() {
  LOG(logDomain, logLevel, prefix + buffer);
}
