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

#include "Base.h"
#include "Event.h"
#include "Transfer.h"
#include "Buffer.h"

#include <cbang/StdTypes.h>
#include <cbang/util/Progress.h>

#include <queue>
#include <functional>


namespace cb {
  namespace Event {
    class Base;
    class FDPool;

    class FD : public RefCounted {
      Base &base;
      int fd;
      SmartPointer<SSL> ssl;

      SmartPointer<Event> completeEvent;

      unsigned readTimeout   = 0;
      unsigned writeTimeout  = 0;
      Progress readProgress  = 60;
      Progress writeProgress = 60;

      std::function<void ()> onClose;

    public:
      enum {
        READ_EVENT    = 1,
        WRITE_EVENT   = 2,
        CLOSE_EVENT   = 4,
        TIMEOUT_EVENT = 8,
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

      std::function<void ()> getOnClose() const {return onClose;}
      void setOnClose(std::function<void ()> cb) {onClose = cb;}

      Progress getReadProgress() const;
      Progress getWriteProgress() const;

      void read(const SmartPointer<Transfer> transfer);
      void read(Transfer::cb_t cb, const Buffer &buffer, unsigned length,
                const std::string &until = std::string());
      void canRead(Transfer::cb_t cb);

      void write(const SmartPointer<Transfer> transfer);
      void write(Transfer::cb_t cb, const Buffer &buffer);
      void canWrite(Transfer::cb_t cb);

      void close();
    };
  }
}
