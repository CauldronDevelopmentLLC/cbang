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

#include "InputSource.h"
#include "BufferDevice.h"
#include "File.h"

#include <cbang/os/SystemUtilities.h>
#include <cbang/iostream/ArrayDevice.h>
#include <cbang/util/Resource.h>

#include <sstream>

using namespace std;
using namespace cb;


InputSource::InputSource(IOBuffer &buffer, const string &name) :
  Named(name), stream(new BufferStream(buffer)), length(buffer.getFill()) {}


InputSource::InputSource(const char *array, streamsize length,
                         const string &name) :
  Named(name), stream(new ArrayStream<const char>(array, length)),
  length(length) {}


InputSource::InputSource(const string &filename) :
  Named(filename), stream(SystemUtilities::iopen(filename)), length(-1) {}


InputSource::InputSource(istream &stream, const string &name,
                         streamsize length) :
  Named(name), stream(SmartPointer<istream>::Phony(&stream)), length(length) {}


InputSource::InputSource(const SmartPointer<istream> &stream,
                         const string &name, streamsize length) :
  Named(name), stream(stream), length(length) {}


InputSource::InputSource(const Resource &resource) :
  Named(resource.getName()),
  stream(new ArrayStream<const char>(resource.getData(), resource.getLength()))
{}


streamsize InputSource::getLength() const {
  if (length == -1 && stream.isInstance<io::stream<FileDevice>>())
    return (*stream.cast<io::stream<FileDevice>>())->size();

  return length;
}


string InputSource::toString() const {
  ostringstream str;
  SystemUtilities::cp(getStream(), str);
  return str.str();
}


string InputSource::getLine(unsigned maxLength) const {
  SmartPointer<char>::Array line = new char[maxLength];
  getStream().getline(line.get(), maxLength);
  return string(line.get());
}
