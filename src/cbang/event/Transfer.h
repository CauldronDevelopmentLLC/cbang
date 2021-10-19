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

#include <cbang/openssl/SSL.h>

#include <functional>


namespace cb {
  namespace Event {
    class Transfer {
    public:
      typedef std::function<void (bool)> cb_t;

    protected:
      int fd;
      SmartPointer<SSL> ssl;
      cb_t cb;
      unsigned length;
      bool finished = false;
      bool success = false;
      uint64_t timeout = 0;

    public:
      Transfer(int fd, const SmartPointer<SSL> &ssl, cb_t cb,
               unsigned length = 0) :
        fd(fd), ssl(ssl), cb(cb), length(length) {}

      virtual ~Transfer() {}

      int getFD() const {return fd;}
      const SmartPointer<SSL> &getSSL() const {return ssl;}
      unsigned getLength() const {return length;}
      bool isFinished() {return finished;}

      void setTimeout(uint64_t timeout) {this->timeout = timeout;}
      uint64_t getTimeout() const {return timeout;}

#ifdef HAVE_OPENSSL
      bool wantsRead() const {return ssl.isSet() && ssl->wantsRead();}
      bool wantsWrite() const {return ssl.isSet() && ssl->wantsWrite();}
#else // HAVE_OPENSSL
      bool wantsRead() const {return false;}
      bool wantsWrite() const {return false;}
#endif // HAVE_OPENSSL

      virtual bool isPending() const {return false;}
      virtual int transfer() {finished = success = true; return 0;}
      virtual void complete() {if (cb) cb(success);}
    };
  }
}
