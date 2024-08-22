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

#include "TailFileToLog.h"

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SysError.h>
#include <cbang/event/Base.h>

#include <cstring>

using namespace std;
using namespace cb;


TailFileToLog::TailFileToLog(
  Event::Base &base, const string &filename,
  const string &prefix, const char *logDomain, unsigned logLevel) :
  event(base.newEvent(this, &TailFileToLog::update)),
  filename(filename), prefix(prefix), logDomain(logDomain),
  logLevel(logLevel) {event->add(0.25);}


void TailFileToLog::update() {
  if (stream.isNull() && SystemUtilities::exists(filename))
    stream = SystemUtilities::iopen(filename.c_str());

  if (stream.isNull()) return;

  for (unsigned i = 0; i < 1e6; i++) {
    if (!stream->good()) {
      LOG_ERROR("Bad stream: " << SysError());
      event->del();
      return;
    }

    int c = stream->get();

    if (c == istream::traits_type::eof()) {
      stream->clear();
      stream->seekg(0, ios::cur); // Reset stream
      return;
    }

    bytes++;

    if (c == '\r') continue;
    if (c != '\n') buffer[fill++] = c;

    if (fill == bufferSize || c == '\n') {
      buffer[fill] = 0;
      fill = 0;
      LOG(logDomain, logLevel, prefix + buffer);
    }
  }
}
