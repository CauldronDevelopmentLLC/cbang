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
#include "Event.h"
#include "Buffer.h"

#include <functional>


namespace cb {
  namespace Event {
    class Websocket : public Request {
      bool active = false;

      Buffer input;
      uint64_t bytesToRead = 0;
      WebsockOpCode wsOpCode;
      uint8_t wsMask[4];
      bool wsFinish = false;
      std::vector<char> wsMsg;

      std::string pongPayload;
      SmartPointer<Event> pingEvent;
      SmartPointer<Event> pongEvent;

      uint64_t msgSent = 0;
      uint64_t msgReceived = 0;

    public:
      Websocket(const URI &uri = URI(), const Version &version = Version(1, 1));

      bool isActive() const;

      uint64_t getMessagesSent() const {return msgSent;}
      uint64_t getMessagesReceived() const {return msgReceived;}

      void send(const char *data, unsigned length);
      void send(const std::string &s);
      void send(const char *s) {send(std::string(s));}

      void close(WebsockStatus status, const std::string &msg = "");
      void ping(const std::string &payload = "");

      // Called by HTTPConnIn
      bool upgrade();

      void readHeader();
      void readBody();

      // From Request
      bool isWebsocket() const {return active;}
      void onResponse(ConnectionError error);

      // Callbacks
      virtual bool onUpgrade() {return true;}
      virtual void onOpen() {}
      virtual void onMessage(const char *data, uint64_t length) = 0;
      virtual void onPing(const std::string &payload);
      virtual void onPong(const std::string &payload);
      virtual void onClose(WebsockStatus status, const std::string &msg) {}

    protected:
      using Request::send;
      using Request::reply;

      void writeRequest(Buffer &buf);

      void writeFrame(WebsockOpCode opcode, bool finish,
                      const void *data, uint64_t len);
      void pong();
      void schedulePong();
      void schedulePing();
      void message(const char *data, uint64_t length);
    };

    typedef SmartPointer<Websocket> WebsocketPtr;
  }
}
