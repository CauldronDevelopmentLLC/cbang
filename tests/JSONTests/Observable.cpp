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

#include <cbang/Catch.h>
#include <cbang/String.h>
#include <cbang/json/Value.h>
#include <cbang/json/Reader.h>
#include <cbang/json/Observable.h>
#include <cbang/json/List.h>
#include <cbang/json/Path.h>

#include <iostream>

using namespace std;
using namespace cb;
using namespace cb::JSON;


class A : public ObservableDict {
public:
  // From ObservableDict
  void notify(const list<ValuePtr> &change) override {
    auto changes = SmartPtr(new JSON::List(change.begin(), change.end()));
    cout << *changes << endl;
  }
};


int main(int argc, char *argv[]) {
  try {
    A a;

    for (int i = 1; i < argc - 1; i += 2)
      Path(argv[i]).insert(a, Reader::parseString(argv[i + 1]));

    cout << "FINAL: " << a << endl;

    return 0;

  } CBANG_CATCH_ERROR;

  return 0;
}
