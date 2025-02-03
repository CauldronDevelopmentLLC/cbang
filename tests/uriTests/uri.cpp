/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include <cbang/Exception.h>
#include <cbang/net/URI.h>

#include <exception>
#include <iostream>
#include <iomanip>

using namespace cb;
using namespace std;

int main(int argc, char *argv[]) {
  try {
    unsigned int w = 10;

    for (int i = 1; i < argc; i++) {
      cout << "" << argv[i] << " => ";

      try {
        URI uri(argv[i]);

        cout << "{" << endl;
        cout << setw(w) << "URI: " << uri << endl;
        cout << setw(w) << "Scheme: " << uri.getScheme() << endl;
        cout << setw(w) << "Host: " << uri.getHost() << endl;
        cout << setw(w) << "Port: " << uri.getPort() << endl;
        cout << setw(w) << "Path: " << uri.getPath() << endl;
        cout << setw(w) << "User: " << uri.getUser() << endl;
        cout << setw(w) << "Pass: " << uri.getPass() << endl;
        cout << setw(w) << "Query: ";
        for (URI::iterator it = uri.begin(); it != uri.end(); it++) {
          if (it != uri.begin()) cout << ", ";
          cout << it->first << "='" << it->second << "'";
        }
        cout << endl;

        cout << "}";

      } catch (const Exception &e) {
        cout << "INVALID: " << e.getMessage() << endl;
      }

      cout << endl;
    }

    return 0;

  } catch (const Exception &e) {
    cerr << "Exception: " << e << endl;

  } catch (const std::exception &e) {
    cerr << "std::exception: " << e.what() << endl;
  }

  return 1;
}

