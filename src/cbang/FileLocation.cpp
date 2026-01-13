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

#include "FileLocation.h"
#include "String.h"

#include <cbang/json/Sink.h>

#include <iostream>

using namespace std;
using namespace cb;


string FileLocation::getFileLineColumn() const {
  string s = filename;

  if (0 <= line) {
    s += ":" + String(line);
    if (0 <= col) s += ":" + String(col);
  }

  return s;
}


bool FileLocation::isEmpty() const {
  return filename.empty() && function.empty() && line < 0 && col < 0;
}


bool FileLocation::operator==(const FileLocation &o) const {
  return filename == o.filename && line == o.line && col == o.col &&
    function == o.function;
}


void FileLocation::print(ostream &stream) const {
  if (isEmpty()) return;
  stream << getFileLineColumn();
  if (!function.empty()) stream << ':' << function << "()";
}


void FileLocation::write(JSON::Sink &sink) const {
  sink.beginDict();
  if (!filename.empty()) sink.insert("filename", filename);
  if (!function.empty()) sink.insert("function", function);
  if (0 <= line) sink.insert("line", line);
  if (0 <= col) sink.insert("column", col);
  sink.endDict();
}
