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

#include "EventFlag.h"
#include "Buffer.h"

#include <cbang/SmartPointer.h>
#include <cbang/socket/SocketType.h>

#include <string>

struct ssl_st;


namespace cb {
  class SSLContext;
  class SSL;
  class IPAddress;
  class Socket;

  namespace Event {
    class Base;
    class Event;
    class DNSBase;
    class DNSRequest;

    class BufferEvent : virtual public RefCounted, public EventFlag {
      Base &base;

      static uint64_t nextID;
      uint64_t id = ++nextID;

      SmartPointer<Socket> socket;

      Buffer inputBuffer;
      Buffer outputBuffer;

      SmartPointer<Event> readEvent;
      SmartPointer<Event> writeEvent;

      SmartPointer<Event> connectCBEvent;
      SmartPointer<Event> writeCBEvent;
      SmartPointer<Event> readCBEvent;
      SmartPointer<Event> errorCBEvent;
      SmartPointer<DNSRequest> dnsReq;

      unsigned readTimeout = 50;
      unsigned writeTimeout = 50;

      unsigned minRead = 0;

      typedef enum {
        STATE_FAILED,
        STATE_IDLE,
        STATE_DNS_LOOKUP,
        STATE_SOCK_CONNECT,
        STATE_SOCK_READY,
        STATE_SSL_HANDSHAKE,
        STATE_SSL_READY,
      } state_t;

      state_t state;
      int peerPort = 0;
      int sslWant = 0;
      bool enableRead = false;

      ssl_st *ssl = 0;
      size_t sslLastWrite = 0;
      size_t sslLastRead = 0;

      short pendingErrorFlags = 0;
      int pendingError = 0;
      std::vector<int> sslErrors;

    public:
      enum {
        BUFFEREVENT_READING = 1 << 0,
        BUFFEREVENT_WRITING = 1 << 1,
        BUFFEREVENT_TIMEOUT = 1 << 2,
        BUFFEREVENT_ERROR   = 1 << 3,
        BUFFEREVENT_EOF     = 1 << 4,
      };


      BufferEvent(Base &base, bool incoming,
                  const SmartPointer<Socket> &socket = 0,
                  const SmartPointer<SSLContext> &sslCtx = 0);
      virtual ~BufferEvent();

      uint64_t getID() const {return id;}

      const Buffer &getInput() const {return inputBuffer;}
      const Buffer &getOutput() const {return outputBuffer;}
      Buffer &getInput() {return inputBuffer;}
      Buffer &getOutput() {return outputBuffer;}

      void setSocket(const SmartPointer<Socket> &);
      const SmartPointer<Socket> &getSocket() const;

      int getPriority() const;
      void setPriority(int priority);

      void setTimeouts(unsigned read, unsigned write);

      bool hasSSL() const {return ssl;}
      SSL getSSL() const;
      void logSSLErrors();
      std::string getSSLErrors();

      void setRead(bool enable);
      void setMinRead(unsigned bytes) {minRead = bytes;}

      static std::string getEventsString(short events);

      void close();
      void connect(DNSBase &dns, const IPAddress &peer);

      virtual void connectCB() {}
      virtual void readCB() {}
      virtual void writeCB() {}
      virtual void errorCB(short what, int err) {}

      void dnsCB(int err, const std::vector<IPAddress> &addrs);

    private:
      void errorCB();
      void scheduleErrorCB(int flags, int err = 0);
      void scheduleReadCB();
      void outputBufferCB(int added, int deleted, int orig);

      void sockRead();
      void sockConnect();
      void sockWrite();

      void sockReadCB(unsigned events);
      void sockWriteCB(unsigned events);

      void sslClosed(unsigned when, int errcode, int ret);
      void sslError(unsigned event, int ret);
      void sslRead();
      void sslWrite();
      void sslHandshake();

      socket_t getFD() const;
      void setFD(socket_t fd);

      // Event control
      void enableEvents(unsigned events = ~0);
      void disableEvents(unsigned events = ~0);
      void updateEventsSSLWant();
      void updateEvents();

      SmartPointer<Event> newEvent(socket_t fd, unsigned event, int priority,
                                   void (BufferEvent::*member)(unsigned));
      SmartPointer<Event> newEvent(void (BufferEvent::*member)());
    };
  }
}
