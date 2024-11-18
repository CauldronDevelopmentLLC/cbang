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

#include <cbang/SmartPointer.h>
#include <cbang/Catch.h>

using namespace cb;
using namespace std;

class A {
public:
  A() {cout << "A()" << endl;}
  ~A() {cout << "~A()" << endl;}
};

template <typename PTR>
void log_refs(const PTR &ptr) {
  cout
    << "refs="  << ptr.getRefCount()
    << " weak=" << ptr.getRefCount(true)
    << " set="  << ptr.isSet()
    << endl;
}

int main(int argc, char *argv[]) {
  try {
    cout << "a = new A" << endl;
    auto a = SmartPtr(new A);
    log_refs(a);

    cout << endl << "b = a" << endl;
    auto b = a;
    log_refs(a);

    cout << endl << "w = WeakPtr(a)" << endl;
    auto w = WeakPtr(a);
    log_refs(a);

    cout << endl << "w.release()" << endl;
    w.release();
    log_refs(a);

    cout << endl << "x = WeakPtr(a)" << endl;
    auto x = WeakPtr(a);
    log_refs(a);

    cout << endl << "w = x" << endl;
    w = x;
    log_refs(a);

    cout << endl << "a.release()" << endl;
    a.release();
    log_refs(x);

    cout << endl << "a = w" << endl;
    a = w;
    log_refs(x);

    cout << endl << "a.release()" << endl;
    a.release();
    log_refs(x);

    cout << endl << "b.release()" << endl;
    b.release();
    log_refs(x);

    cout << endl << "w.release()" << endl;
    w.release();
    log_refs(x);

    cout << endl << "x.release()" << endl;
    x.release();
    log_refs(x);

    return 0;
  } CATCH_ERROR;

  return 1;
}
