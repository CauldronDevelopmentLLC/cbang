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

#pragma once

#include <cbang/Exception.h>

#include <cbang/Catch.h>
#include <cbang/openssl/SSL.h>

#ifdef _WIN32
#include <cbang/os/win/Win32EventLog.h>
#endif


namespace cb {
#ifndef WINAPI
#define WINAPI
#endif

  template <class T>
  static int WINAPI doApplication(int argc, char *argv[]) {
    try {
#ifdef HAVE_OPENSSL
      SSL::init();
#endif // HAVE_OPENSSL

      T app;
      int i = app.init(argc, argv);
      if (i < 0) return i;

      app.run();

      return 0;

    } catch (const Exception &e) {
      std::string msg = SSTR("Exception: " << e CBANG_CATCH_LOCATION);
      CBANG_LOG_ERROR(msg);
#ifdef _WIN32
      Win32EventLog(argv[0]).log(msg);
#endif // _WIN32
      if (e.getCode()) return e.getCode();
    }

    return 1;
  }
}
