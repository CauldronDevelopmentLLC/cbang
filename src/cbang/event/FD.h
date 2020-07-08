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
#include "Transport.h"
#include "Transfer.h"

#include <cbang/StdTypes.h>
#include <cbang/os/Mutex.h>
#include <cbang/util/Progress.h>

#include <queue>
#include <functional>


namespace cb {
  namespace Event {
    class Base;
    class FDPool;

    class FD : public RefCounted, protected Mutex {
      Base &base;
      int fd;
      SmartPointer<SSL> ssl;
      SmartPointer<Transport> transport;

      SmartPointer<Event> completeEvent;
      SmartPointer<Event> timeoutEvent;

      bool inPool            = false;
      unsigned lastEvents    = 0;

      unsigned readTimeout   = 0;
      unsigned writeTimeout  = 0;
      uint64_t lastRead      = 0;
      uint64_t lastWrite     = 0;
      Progress readProgress  = 60;
      Progress writeProgress = 60;

      std::function<void ()> onComplete;

      typedef std::queue<SmartPointer<Transfer> > queue_t;
      queue_t readQ;
      queue_t writeQ;
      queue_t completeQ;

      bool readTimedout   = false;
      bool writeTimedout  = false;
      bool readClosed     = false;
      bool writeClosed    = false;
      bool readWantsWrite = false;
      bool writeWantsRead = false;

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

      void setPriority(int priority);
      int getPriority() const;

      unsigned getReadTimeout() const {return readTimeout;}
      void setReadTimeout(unsigned timeout);
      bool isReadTimedout() const {return readTimedout;}

      unsigned getWriteTimeout() const {return writeTimeout;}
      void setWriteTimeout(unsigned timeout);
      bool isWriteTimedout() const {return writeTimedout;}

      bool isTimedout() const {return readTimedout || writeTimedout;}

      std::function<void ()> getOnComplete() const {return onComplete;}
      void setOnComplete(std::function<void ()> cb) {onComplete = cb;}

      bool isClosed() const {return readClosed && writeClosed;}
      bool isReadClosed() const {return readClosed;}
      bool isWriteClosed() const {return writeClosed;}

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
      void transfer(unsigned events);

    protected:
      void flush(queue_t &q, bool success);
      int transfer(queue_t &q);
      void read(unsigned events);
      void write(unsigned events);

      bool readTimeoutValid() const;
      bool writeTimeoutValid() const;

      static std::string eventsToString(unsigned events);
      unsigned getEvents() const;
      void updateEvents();
      void updateTimeout();
      void update();

      void complete();
      void timeout();

      void poolRemove();
      void poolAdd();
    };
  }
}
