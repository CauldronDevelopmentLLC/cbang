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

#include "Request.h"
#include "BufferEvent.h"
#include "Enum.h"

#include <cbang/SmartPointer.h>
#include <cbang/net/IPAddress.h>
#include <cbang/socket/SocketType.h>

#include <limits>
#include <list>
#include <vector>


namespace cb {
  class URI;
  class SSLContext;
  class Socket;
  class RateSet;

  namespace Event {
    class Event;
    class HTTP;
    class Request;
    class Websocket;

    class Connection : public BufferEvent, public Enum {
      Base &base;

      typedef enum {
        STATE_DISCONNECTED,
        STATE_CONNECTING,
        STATE_IDLE,
        STATE_READING_FIRSTLINE,
        STATE_READING_HEADERS,
        STATE_READING_BODY,
        STATE_READING_TRAILER,
        STATE_WEBSOCK_HEADER,
        STATE_WEBSOCK_BODY,
        STATE_WRITING,
      } state_t;

      state_t state;
      bool incoming;
      IPAddress peer;
      IPAddress bind;
      double startTime;
      SmartPointer<HTTP> http;
      SmartPointer<SSLContext> sslCtx;

      unsigned retries = 0;
      SmartPointer<Event> retryEvent;
      std::list<SmartPointer<Request> > requests;

      std::string defaultContentType = "text/html; charset=UTF-8";
      uint32_t maxBodySize = std::numeric_limits<unsigned>::max();
      uint32_t maxHeaderSize = std::numeric_limits<unsigned>::max();
      double retryTimeout = 0;
      unsigned maxRetries = 2;
      unsigned readTimeout = 50;
      unsigned writeTimeout = 50;
      unsigned connectTimeout = 50;

      bool detectClose = false;
      bool chunkedRequest = false;

      uint32_t headerSize = 0;
      uint32_t bodySize = 0;
      int64_t bytesToRead = 0;
      int contentLength = 0;

      SmartPointer<RateSet> stats;

    public:
      Connection(Base &base, bool incoming, const IPAddress &peer,
                 const SmartPointer<Socket> &socket = 0,
                 const SmartPointer<SSLContext> &sslCtx = 0);
      virtual ~Connection();

      Base &getBase() {return base;}
      virtual DNSBase &getDNS() {THROW("No DNSBase");}

      bool isConnected() const;
      bool isIncoming() const {return incoming;}

      const cb::IPAddress &getPeer() const {return peer;}
      void setPeer(const cb::IPAddress &peer) {this->peer = peer;}

      const cb::IPAddress &getLocalAddress() const {return bind;}
      void setLocalAddress(const cb::IPAddress &bind) {this->bind = bind;}

      double getStartTime() const {return startTime;}

      const SmartPointer<HTTP> &getHTTP() const {return http;}
      void setHTTP(const SmartPointer<HTTP> &http) {this->http = http;}

      const SmartPointer<Request> &getRequest() const;
      void checkActiveRequest(Request &req) const;
      bool isWebsocket() const;
      Websocket &getWebsocket() const;

      const std::string &getDefaultContentType() const
        {return defaultContentType;}
      void setDefaultContentType(const std::string &s) {defaultContentType = s;}

      void setMaxBodySize(uint32_t size) {maxBodySize = size;}
      uint32_t getMaxBodySize() const {return maxBodySize;}

      void setMaxHeaderSize(uint32_t size) {maxHeaderSize = size;}
      uint32_t getMaxHeaderSize() const {return maxHeaderSize;}

      void setMaxRetries(unsigned retries) {maxRetries = retries;}
      unsigned getMaxRetries() const {return maxRetries;}

      void setReadTimeout(unsigned t) {readTimeout = t;}
      unsigned getReadTimeout() const {return readTimeout;}

      void setWriteTimeout(unsigned t) {writeTimeout = t;}
      unsigned getWriteTimeout() const {return writeTimeout;}

      void setConnectTimeout(unsigned t) {connectTimeout = t;}
      unsigned getConnectTimeout() const {return connectTimeout;}

      uint32_t getHeaderSize() const {return headerSize;}
      uint32_t getBodySize() const {return bodySize;}

      void setStats(const SmartPointer<RateSet> &stats) {this->stats = stats;}
      const SmartPointer<RateSet> &getStats() const {return stats;}

      void sendServiceUnavailable();
      void makeRequest(Request &req);
      void acceptRequest();
      void cancelRequest(Request &req);
      void write(Request &req, const Buffer &buf);

    protected:
      static const char *getStateString(state_t state);
      void setState(state_t state);

      void push(const SmartPointer<Request> &req);
      SmartPointer<Request> pop();

      void free(ConnectionError error);
      void fail(ConnectionError error);
      void reset();
      void dispatch();
      void done();
      void retry();
      void connect();

      void websockClose(WebsockStatus status, const std::string &msg = "");
      void websockReadHeader();
      void websockReadBody();

      void startRead();
      void newRequest(const std::string &line);
      void readFirstLine();
      bool tryReadHeader();
      void headersCallback();
      void readHeader();
      void getBody();
      void readBody();
      void readTrailer();

      // From BufferEvent
      using BufferEvent::setTimeouts;
      using BufferEvent::setSocket;
      using BufferEvent::getSocket;
      using BufferEvent::setRead;
      using BufferEvent::getOutput;
      using BufferEvent::getInput;
      using BufferEvent::connect;
      using BufferEvent::close;

      void connectCB();
      void readCB();
      void writeCB();
      void errorCB(short what, int err);

      void received(unsigned bytes);
      void sent(unsigned bytes);
    };
  }
}
