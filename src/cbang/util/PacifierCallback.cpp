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

#include "PacifierCallback.h"

#include <cbang/Application.h>

#include <cbang/time/Time.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;


PacifierCallback::PacifierCallback(Application &app, streamsize total,
                                   const string &prefix, unsigned rate,
                                   unsigned level) :
  app(app), prefix(prefix), rate(rate), level(level), completed(0),
  total(total), last(Time::now()) {}


bool PacifierCallback::transferCallback(streamsize count) {
  completed += count;

  if (last + rate < Time::now()) {
    last = Time::now();

    double percent = (double)completed / total * 100;
    if (0.0051 < percent)
      LOG_INFO(level, prefix << fixed << setprecision(2) << percent << "%");
  }

  return !app.shouldQuit();
}
