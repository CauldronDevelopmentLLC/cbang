/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#pragma once

#include <cstdint>


namespace cb {
  class PowerManagement {
    static PowerManagement *singleton;

    uint64_t lastBatteryUpdate     = 0;
    bool systemOnBattery           = false;
    bool systemHasBattery          = false;

    uint64_t lastIdleSecondsUpdate = 0;
    unsigned idleSeconds           = 0;

    bool systemSleepAllowed        = true;


  protected:
    PowerManagement() {}
    virtual ~PowerManagement() {}

  public:
    static PowerManagement &instance();

    bool onBattery();
    bool hasBattery();
    unsigned getIdleSeconds();
    void allowSystemSleep(bool x);

  protected:
    void updateIdleSeconds();
    void updateBatteryInfo();

    virtual void _setAllowSleep(bool allow) = 0;
    virtual unsigned _getIdleSeconds() = 0;
    virtual bool _getHasBattery() = 0;
    virtual bool _getOnBattery() = 0;
  };
}
