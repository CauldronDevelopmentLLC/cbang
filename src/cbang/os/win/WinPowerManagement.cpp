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

#include "WinPowerManagement.h"

#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

using namespace cb;


WinPowerManagement::~WinPowerManagement() {_setAllowSleep(true);}


void WinPowerManagement::_setAllowSleep(bool allow) {
  SetThreadExecutionState(
    ES_CONTINUOUS |
    (allow ? 0 : (ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED)));
}


unsigned WinPowerManagement::_getIdleSeconds() {
  LASTINPUTINFO lif;
  lif.cbSize = sizeof(LASTINPUTINFO);
  GetLastInputInfo(&lif);

  return (GetTickCount() - lif.dwTime) / 1000; // Convert from ms.
}


bool WinPowerManagement::_getHasBattery() {
  SYSTEM_POWER_STATUS status;
  return GetSystemPowerStatus(&status) && !(status.BatteryFlag & 128);
}


bool WinPowerManagement::_getOnBattery() {
  SYSTEM_POWER_STATUS status;
  return GetSystemPowerStatus(&status) && !status.ACLineStatus;
}
