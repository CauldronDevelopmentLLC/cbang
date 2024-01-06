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

#include <cbang/net/IPRangeSet.h>

#include <iostream>

using namespace cb;
using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 2 && argc != 3) {
    cerr << "Usage: " << argv[0] << " <IP ranges> [test IP]" << endl;
    return 1;
  }

  IPRangeSet set(argv[1]);
  cout << set << endl;

  if (argc == 3) {
    bool contains = set.contains(IPAddress(argv[2]));
    cout << (contains ? "true" : "false") << endl;
    return contains ? 0 : 1;
  }

  return 0;
}
