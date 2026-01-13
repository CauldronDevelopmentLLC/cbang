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

#pragma once

#include <cbang/config.h>
#include <cbang/SmartPointer.h>
#include <cbang/boost/IOStreams.h>

#include <string>


namespace cb {
  class Logger;

  class LogDevice {
  public:
    typedef char char_type;
    struct category : io::sink_tag, io::flushable_tag {};

    class impl {
      std::string prefix;
      std::string suffix;
      std::string trailer;
      std::string rateKey;

      std::string buffer;
      std::string rateMessage;
      bool first = true;
      bool startOfLine = true;
      StackTrace trace;

    public:
      impl(const std::string &prefix, const std::string &suffix,
           const std::string &trailer,
           const std::string &rateKey = std::string());
      ~impl();

      std::streamsize write(const char_type *s, std::streamsize n);
      void flushLine();
      bool flush();
    };

  protected:
    SmartPointer<impl> _impl;

  public:
    LogDevice(const SmartPointer<impl> &_impl) : _impl(_impl) {}
    LogDevice(const std::string &prefix,
              const std::string &suffix  = std::string(),
              const std::string &trailer = std::string()) :
      _impl(new impl(prefix, suffix, trailer)) {}

    std::streamsize write(const char_type *s, std::streamsize n)
    {return _impl->write(s, n);}
    bool flush() {return _impl->flush();}
  };

  typedef io::stream<LogDevice> LogStream;
}
