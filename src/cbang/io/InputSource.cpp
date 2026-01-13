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

#include "InputSource.h"
#include "ArrayStream.h"

#include <cbang/os/SystemUtilities.h>
#include <cbang/util/Resource.h>

#include <sstream>
#include <cstring>

using namespace std;
using namespace cb;


InputSource::InputSource(
  const char *array, streamsize length, const string &name) :
  Named(name), stream(
    new ArrayStream<const char>(array, length < 0 ? strlen(array) : length)) {}


InputSource::InputSource(const string &s, const string &name) :
  InputSource(s.data(), s.length(), name) {}


InputSource::InputSource(istream &stream, const string &name) :
  Named(name), stream(SmartPointer<istream>::Phony(&stream)) {}


InputSource::InputSource(
  const SmartPointer<istream> &stream, const string &name) :
  Named(name), stream(stream) {}


InputSource::InputSource(const Resource &resource) :
  InputSource(resource.getData(), resource.getLength(), resource.getName()) {}


InputSource InputSource::open(const string &filename) {
  return InputSource(SystemUtilities::iopen(filename), filename);
}


string InputSource::toString() const {
  ostringstream str;
  SystemUtilities::cp(*stream, str);
  return str.str();
}


string InputSource::getLine(unsigned maxLength) const {
  SmartPointer<char>::Array line = new char[maxLength];
  stream->getline(line.get(), maxLength);
  return string(line.get());
}
