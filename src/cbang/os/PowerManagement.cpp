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

#include "PowerManagement.h"

#include "win/WinPowerManagement.h"
#include "lin/LinPowerManagement.h"
#include "osx/MacOSPowerManagement.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>
#include <cbang/time/Time.h>

#include <cstring>

using namespace cb;
using namespace std;


PowerManagement *PowerManagement::singleton = 0;


PowerManagement &PowerManagement::instance() {
  if (!singleton) {
#if defined(_WIN32)
    singleton = new WinPowerManagement;
#elif defined(__APPLE__)
    singleton = new MacOSPowerManagement;
#else
    singleton = new LinPowerManagement;
#endif
  }

  return *singleton;
}


bool PowerManagement::onBattery() {
  updateBatteryInfo();
  return systemOnBattery;
}


bool PowerManagement::hasBattery() {
  updateBatteryInfo();
  return systemHasBattery;
}


unsigned PowerManagement::getIdleSeconds() {
  updateIdleSeconds();
  return idleSeconds;
}


void PowerManagement::allowSystemSleep(bool x) {
  if (systemSleepAllowed == x) return;
  systemSleepAllowed = x;

  LOG_DEBUG(5, (x ? "Allowing" : "Disabling") << " system sleep");
  _setAllowSleep(systemSleepAllowed);
}


void PowerManagement::updateIdleSeconds() {
  // Avoid checking this too often
  if (Time::now() <= lastIdleSecondsUpdate) return;
  lastIdleSecondsUpdate = Time::now();
  idleSeconds = _getIdleSeconds();
}


void PowerManagement::updateBatteryInfo() {
  // Avoid checking this too often
  if (Time::now() <= lastBatteryUpdate) return;
  lastBatteryUpdate = Time::now();

  try {
    systemHasBattery = _getHasBattery();
    systemOnBattery  = systemHasBattery && _getOnBattery();
  } CBANG_CATCH_ERROR;
}
