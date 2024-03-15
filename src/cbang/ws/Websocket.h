/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "Status.h"
#include "OpCode.h"
#include "Enum.h"

#include <cbang/event/Event.h>
#include <cbang/event/Buffer.h>
#include <cbang/http/Request.h>

#include <functional>


namespace cb {
  namespace WS {
    class Websocket : public HTTP::Request, public Enum {
      bool active = false;

      Event::Buffer input;
      uint64_t bytesToRead = 0;
      OpCode wsOpCode;
      uint8_t wsMask[4];
      bool wsFinish = false;
      std::vector<char> wsMsg;

      std::string pongPayload;
      SmartPointer<Event::Event> pingEvent;
      SmartPointer<Event::Event> pongEvent;

      uint64_t msgSent = 0;
      uint64_t msgReceived = 0;

    public:
      Websocket(const SmartPointer<HTTP::Conn> &connection = 0,
                const URI &uri = URI(), const Version &version = Version(1, 1));

      bool isActive() const;

      uint64_t getMessagesSent() const {return msgSent;}
      uint64_t getMessagesReceived() const {return msgReceived;}

      // From Request
      void send(const char *data, unsigned length) override;
      void send(const std::string &s) override;
      void send(const char *s) override {send(std::string(s));}

      void close(Status status, const std::string &msg = "");
      void ping(const std::string &payload = "");

      // Called by HTTP::ConnIn
      bool upgrade();

      void readHeader();
      void readBody();

      // From Request
      bool isWebsocket() const override {return active;}
      void onResponse(Event::ConnectionError error) override;

      // Callbacks
      virtual bool onUpgrade() {return true;}
      virtual void onOpen() {}
      virtual void onMessage(const char *data, uint64_t length) = 0;
      virtual void onPing(const std::string &payload);
      virtual void onPong(const std::string &payload);
      virtual void onClose(Status status, const std::string &msg) {}

    protected:
      using HTTP::Request::send;
      using HTTP::Request::reply;

      // From Request
      void writeRequest(Event::Buffer &buf) override;

      void writeFrame(
        OpCode opcode, bool finish, const void *data, uint64_t len);
      void pong();
      void schedulePong();
      void schedulePing();
      void message(const char *data, uint64_t length);
    };

    typedef SmartPointer<Websocket> WebsocketPtr;
  }
}
