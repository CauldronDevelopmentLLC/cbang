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

#pragma once

#include "Base.h"
#include "Transfer.h"
#include "Buffer.h"

#include <cbang/util/Progress.h>
#include <cbang/util/NonCopyable.h>

#include <cstdint>


namespace cb {
  namespace Event {
    class FDPool;

    class FD : public RefCounted, public NonCopyable {
      Base &base;
      int fd = -1;
      SmartPointer<SSL> ssl;

      unsigned readTimeout  = 0;
      unsigned writeTimeout = 0;

      uint64_t start = Time::now();
      Progress readProgress;
      Progress writeProgress;
      int status = 0;

    public:
      enum {
        READ_EVENT    = 1 << 0,
        WRITE_EVENT   = 1 << 1,
        CLOSE_EVENT   = 1 << 2,
        TIMEOUT_EVENT = 1 << 3,
      };

      FD(Base &base, int fd = -1, const SmartPointer<SSL> &ssl = 0);
      virtual ~FD();

      Base &getBase() {return base;}

      void setFD(int fd);
      int getFD() const {return fd;}

      bool isSecure() const {return ssl.isSet();}
      void setSSL(const SmartPointer<SSL> ssl) {this->ssl = ssl;}
      const SmartPointer<SSL> getSSL() const {return ssl;}

      unsigned getReadTimeout() const {return readTimeout;}
      void setReadTimeout(unsigned timeout);

      unsigned getWriteTimeout() const {return writeTimeout;}
      void setWriteTimeout(unsigned timeout);

      uint64_t getStart() const {return start;}

      Progress &getReadProgress() {return readProgress;}
      Progress &getWriteProgress() {return writeProgress;}

      void setStatus(int status) {this->status = status;}
      int getStatus() const {return status;}

      void progressStart(bool read, unsigned size, uint64_t time);
      void progressEvent(bool read, unsigned size, uint64_t time);
      void progressEnd(bool read, unsigned size);

      void read(const SmartPointer<Transfer> transfer);
      void read(Transfer::cb_t cb, const Buffer &buffer, unsigned length,
                const std::string &until = std::string());
      void canRead(Transfer::cb_t cb);

      void write(const SmartPointer<Transfer> transfer);
      void write(Transfer::cb_t cb, const Buffer &buffer);
      void canWrite(Transfer::cb_t cb);

      virtual void close();
    };
  }
}
