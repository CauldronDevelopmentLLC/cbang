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

#include "Directory.h"

#define BOOST_SYSTEM_NO_DEPRECATED
#include <cbang/boost/StartInclude.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <cbang/boost/EndInclude.h>

namespace fs = boost::filesystem;

using namespace std;
using namespace cb;


#define RETHROW_BOOST(EXPR)                     \
  try {                                         \
    EXPR;                                       \
  } catch (const exception &e) {                \
    THROW(e.what());                            \
  }


struct Directory::private_t {
  fs::path path;
  fs::directory_iterator it;

  private_t(const string &_path) :
    path(fs::system_complete(fs::path(_path))), it(path) {}
};


Directory::Directory(const string &path) : dirPath(path) {
  bool isDir;
  RETHROW_BOOST(isDir = fs::is_directory(path));

  if (!isDir) THROW("Not a directory '" << path << "'");
  RETHROW_BOOST(p = new private_t(path));
}


void Directory::rewind() {
  RETHROW_BOOST(p->it = fs::directory_iterator(p->path));
}


Directory::operator bool() const {
  RETHROW_BOOST(return p->it != fs::directory_iterator());
}


void Directory::next() {p->it++;}


string Directory::getFilename() const {
#if BOOST_FILESYSTEM_VERSION < 3
  RETHROW_BOOST(return p->it->path().filename());
#else
  RETHROW_BOOST(return p->it->path().filename().string());
#endif
}


string Directory::getPath() const {return dirPath + "/" + getFilename();}


bool Directory::isSubdirectory() const {
  RETHROW_BOOST(return fs::is_directory(p->it->status()));
}
