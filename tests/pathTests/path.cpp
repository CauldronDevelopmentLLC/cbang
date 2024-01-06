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

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/Catch.h>

#include <iostream>
#include <iomanip>

using namespace cb;
using namespace std;


void usage(const char *name) {
  cout
    << "Usage: " << name << " [OPTIONS]\n\n"
    << "OPTIONS:\n"
    << "\t--help                           Print this help screen and exit.\n"
    << endl;
}


int main(int argc, char *argv[]) {
  try {
    for (int i = 1; i < argc; i++) {
      string arg = argv[i];

      if (arg == "--help") {
        usage(argv[0]);
        return 0;

      } else if (arg == "--basename" && i < argc - 1) {
        cout << "'" << SystemUtilities::basename(argv[++i]) << "'" << endl;

      } else if (arg == "--dirname" && i < argc - 1) {
        cout << "'" << SystemUtilities::dirname(argv[++i]) << "'" << endl;

      } else if (arg == "--extension" && i < argc - 1) {
        cout << "'" << SystemUtilities::extension(argv[++i]) << "'" << endl;

      } else if (arg == "--is_absolute" && i < argc - 1) {
        cout << String(SystemUtilities::isAbsolute(argv[++i])) << endl;

      } else if (arg == "--canonical" && i < argc - 1) {
        cout << String(SystemUtilities::getCanonicalPath(argv[++i])) << endl;

      } else {
        usage(argv[0]);
        THROWS("Invalid arg '" << arg << "'");
      }
    }

    return 0;

  } CATCH_ERROR;

  return 1;
}

