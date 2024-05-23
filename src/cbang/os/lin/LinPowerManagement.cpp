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

#include "LinPowerManagement.h"

#include <cbang/config.h>
#include <cbang/os/SystemUtilities.h>

#include <string>

#if defined(HAVE_SYSTEMD)
#include <systemd/sd-bus.h>
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace cb;
using namespace std;


struct LinPowerManagement::private_t {
#if defined(HAVE_SYSTEMD)
  sd_bus *bus;
  int sleepFD;
#endif
};



LinPowerManagement::LinPowerManagement() :
  hasBattery(!findDevice("Battery").empty()), acPath(findDevice("Mains")) {

#if defined(HAVE_SYSTEMD)
  pri = new private_t;
  memset(pri, 0, sizeof(private_t));
  sd_bus_open_system(&pri->bus);
#endif
}


LinPowerManagement::~LinPowerManagement() {
#if defined(HAVE_SYSTEMD)
  if (pri) {
    pri->bus = sd_bus_flush_close_unref(pri->bus);
    delete pri;
  }
#endif
}


void LinPowerManagement::_setAllowSleep(bool allow) {
#if defined(HAVE_SYSTEMD)
  if (!pri || !pri->bus) return;

  if (0 < pri->sleepFD) close(pri->sleepFD);
  pri->sleepFD = -1;

  if (!allow) {
    sd_bus_message *reply = 0;
    sd_bus_error    error = SD_BUS_ERROR_NULL;
    string who =
      SystemUtilities::basename(SystemUtilities::getExecutablePath());

    if (sd_bus_call_method(
          pri->bus, "org.freedesktop.login1", "/org/freedesktop/login1",
          "org.freedesktop.login1.Manager", "Inhibit", &error, &reply, "ssss",
          "sleep", who.c_str(), "Prevent system sleep", "block") < 0)
      THROW("Failed to prevent sleep: " << error.message);

    int tmpFD = -1;
    sd_bus_message_read_basic(reply, SD_BUS_TYPE_UNIX_FD, &tmpFD);

    if (0 < tmpFD) pri->sleepFD = fcntl(tmpFD, F_DUPFD_CLOEXEC, 3);

    sd_bus_error_free(&error);
    sd_bus_message_unref(reply);
  }
#endif // HAVE_SYSTEMD
}


unsigned LinPowerManagement::_getIdleSeconds() {
#if defined(HAVE_SYSTEMD)
  int idle = 0;

  // logind API does not exactly match Windows and macOS ones,
  // if IdleHint is true, assume enough time without input has passed
  if (!pri->bus || sd_bus_get_property_trivial(
        pri->bus, "org.freedesktop.login1",
        "/org/freedesktop/login1/seat/seat0",
        "org.freedesktop.login1.Seat", "IdleHint", 0, 'b', &idle) < 0)
    return 0;

  return idle ? -1 : 0;
#endif // HAVE_SYSTEMD

  return 0;
}

bool LinPowerManagement::_getOnBattery() {
  return !acPath.empty() && SystemUtilities::exists(acPath + "/online") &&
    String::trim(SystemUtilities::read(acPath + "/online")) == "0";
}


string LinPowerManagement::findDevice(const string &type) const {
  string match;

  auto cb = [&] (const string &path, unsigned depth) {
    if (match.empty() && SystemUtilities::exists(path + "/type") &&
        String::trim(SystemUtilities::read(path + "/type")) == type)
      match = path;
  };

  SystemUtilities::listDirectory("/sys/class/power_supply", cb, ".*", 1, true);

  return match;
}
