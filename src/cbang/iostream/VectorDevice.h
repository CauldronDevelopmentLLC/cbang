/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include <cbang/boost/IOStreams.h>

#include <vector>
#include <cstring>


namespace cb {
  template <typename T = char>
  class VectorDevice {
    std::vector<T> &v;
    std::streamsize readPtr;

  public:
    typedef T char_type;
    typedef io::bidirectional_device_tag category;

    VectorDevice(std::vector<T> &v) : v(v), readPtr(0) {}

    std::streamsize read(char_type *s, std::streamsize n) {
      if ((std::streamsize)v.size() <= readPtr) return -1;
      if ((std::streamsize)v.size() < readPtr + n) n = v.size() - readPtr;

      memcpy((void *)s, (void *)&v[readPtr], n);
      readPtr += n;
      return n;
    }

    std::streamsize write(const char_type *s, std::streamsize n) {
      v.insert(v.end(), s, s + n);
      return n;
    }
  };


  template <typename T = char>
  class VectorStream : public io::stream<VectorDevice<T> > {
  public:
    VectorStream(std::vector<T> &v) : io::stream<VectorDevice<T> >(v) {}
  };
}
