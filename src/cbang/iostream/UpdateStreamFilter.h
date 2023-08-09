/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include <cbang/SmartPointer.h>
#include <cbang/iostream/Boost.h>

#include <iostream>


namespace cb {
  template <typename T, typename TARGET_CHAR_T = char, typename CHAR_T = char>
  class UpdateStreamFilter {
    T &target;

  public:
    typedef CHAR_T char_type;
    struct category : io::dual_use, io::filter_tag, io::multichar_tag {};

    UpdateStreamFilter(T &target) : target(target) {}

    template<typename Source>
    std::streamsize read(Source &src, CHAR_T *s, std::streamsize n) {
      std::streamsize bytes = io::read(src, s, n);
      target.update((TARGET_CHAR_T *)s, bytes);
      return bytes;
    }


    template<typename Sink>
    std::streamsize write(Sink &dst, const CHAR_T *s, std::streamsize n) {
      std::streamsize bytes = io::write(dst, s, n);
      target.update((TARGET_CHAR_T *)s, bytes);
      return bytes;
    }
  };
}
