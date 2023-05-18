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

#include <boost/iostreams/char_traits.hpp> // EOF, WOULD_BLOCK
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/close.hpp>

#include <cbang/SmartPointer.h>
#include <cbang/Exception.h>
#include <cbang/net/Base64.h>
#include <cbang/log/Logger.h>

#include <cstdio>


namespace cb {
  class Base64Encoder :
    public Base64, public boost::iostreams::dual_use_filter {
    int count;
    int state;
    unsigned char a, b, c;

  public:
    Base64Encoder(const Base64 &base64 = Base64()) :
      Base64(base64), count(0), state(0) {}


    template<typename Source> int get(Source &src) {
      int x;

      if (getWidth() && count++ == getWidth()) {
        count = 0;
        return '\n';
      }

      if (state != 3) {
        x = boost::iostreams::get(src);
        if (x == boost::iostreams::WOULD_BLOCK)
          return boost::iostreams::WOULD_BLOCK;

        if (x == EOF && state == 0) return EOF;
      }

      switch (state) {
      case 0:
        a = (unsigned char)x;
        state++;
        return Base64::encode(a >> 2);

      case 1:
        if (x == EOF) {
          b = 0;
          state = 5;

        } else {
          b = (unsigned char)x;
          state++;
        }

        return Base64::encode(a << 4 | b >> 4);

      case 2:
        if (x == EOF) {
          c = 0;
          state = 6;

        } else {
          c = (unsigned char)x;
          state++;
        }

        return Base64::encode(b << 2 | c >> 6);

      case 3:
        state = 0;
        return Base64::encode(c);

      case 5:
      case 6:
        state++;
        if (getPad()) return getPad();

      default: return EOF;
      }
    }


    template<typename Sink> bool put(Sink &dest, int x) {
      bool ok;

      switch (state) {
      case 0:
        a = (unsigned char)x;
        ok = out(dest, Base64::encode(a >> 2));
        if (ok) state++;
        break;

      case 1:
        b = (unsigned char)x;
        ok = out(dest, Base64::encode(a << 4 | b >> 4));
        if (ok) state++;
        break;

      case 2:
        c = (unsigned char)x;
        ok = out(dest, Base64::encode(b << 2 | c >> 6));
        if (!ok) break;
        ok = out(dest, Base64::encode(c);
        if (!ok) {state++; break;}
        state = 0;
        break;

      case 3:
        if (x != c)
          THROW("Output character changed from '" << c << "' to '"
                 << (unsigned char)x << "'");
        ok = out(dest, Base64::encode(c));
        if (ok) state = 0;
        break;
      }

      return ok;
    }


    template<typename Device> void close(Device &dev, BOOST_IOS::openmode m) {
      char pad = getPad();

      if (m == std::ios::out)
        switch (state) {
        case 1:
          b = 0;
          out(dev, Base64::encode(a << 4 | b >> 4));
          if (pad) out(dev, pad);
          if (pad) out(dev, pad);
          break;

        case 2:
          c = 0;
          out(dev, Base64::encode(b << 2 | c >> 6));
          if (pad) out(dev, pad);
          break;

        case 3:
          out(dev, Base64::encode(c));
          break;
        }

      count = state = 0;
    }

  protected:
    template<typename Sink> bool out(Sink &dest, char x) {
      if (getWidth()) {
        if (count == getWidth()) {
          if (!boost::iostreams::put(dest, '\n')) return false;
          count = 0;

        } else count++;
      }

      return boost::iostreams::put(dest, x);
    }
  };
}
