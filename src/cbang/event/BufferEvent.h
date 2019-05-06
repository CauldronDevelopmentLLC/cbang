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

#include "Socket.h"
#include "Buffer.h"

#include <cbang/SmartPointer.h>

#include <string>

struct bufferevent;


namespace cb {
  class SSLContext;
  class SSL;
  class IPAddress;

  namespace Event {
    class Base;
    class DNSBase;

    class BufferEvent : SmartPointer<BufferEvent>::SelfRef {
      friend class SelfRefCounter;

      bufferevent *bev;

      Buffer inputBuffer;
      Buffer outputBuffer;

    public:
      BufferEvent(Base &base, bool incoming, socket_t fd = -1,
                  const SmartPointer<SSLContext> &sslCtx = 0);
      virtual ~BufferEvent();

      bufferevent *getBufferEvent() const {return bev;}

      const Buffer &getInput() const {return inputBuffer;}
      const Buffer &getOutput() const {return outputBuffer;}
      Buffer &getInput() {return inputBuffer;}
      Buffer &getOutput() {return outputBuffer;}

      void setFD(socket_t fd);
      socket_t getFD() const;

      void setPriority(int priority);
      int getPriority() const;

      void setTimeouts(unsigned read, unsigned write);

      bool hasSSL() const;
      SSL getSSL() const;
      void logSSLErrors();
      std::string getSSLErrors();

      void setRead(bool enabled, bool hard = false);
      void setWrite(bool enabled, bool hard = false);

      void setWatermark(bool read, size_t low, size_t high = 0);

      void connect(DNSBase &dns, const IPAddress &peer);

      virtual void readCB() {}
      virtual void writeCB() {}
      virtual void eventCB(short what) {}

      static std::string getEventsString(short events);
    };
  }
}
