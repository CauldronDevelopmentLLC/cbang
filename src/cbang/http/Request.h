/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Enum.h"
#include "Session.h"
#include "RequestParams.h"

#include <cbang/event/Buffer.h>
#include <cbang/SmartPointer.h>
#include <cbang/util/Version.h>
#include <cbang/net/SockAddr.h>
#include <cbang/json/Value.h>
#include <cbang/json/Writer.h>
#include <cbang/comp/Compression.h>
#include <cbang/debug/Demangle.h>

#include <string>
#include <iostream>


namespace cb {
  class URI;
  class SSL;

  namespace HTTP {
    class Request : virtual public RefCounted, public Enum {
      using HeadersPtr = SmartPointer<Headers>;
      HeadersPtr inputHeaders;
      HeadersPtr outputHeaders;

      Event::Buffer inputBuffer;
      Event::Buffer outputBuffer;

      SmartPointer<Conn>::Weak connection;
      Method method;
      URI uri;
      Version version;
      Status responseCode;
      std::string responseCodeLine;

      Event::ConnectionError connError;

      SmartPointer<Session> session;
      std::string user = "anonymous";

      bool chunked  = false;
      bool replying = false;

      uint64_t bytesRead    = 0;
      uint64_t bytesWritten = 0;

      JSON::ValuePtr args;
      JSON::ValuePtr msg;

    public:
      Request(const RequestParams &params);
      virtual ~Request();

      template <class T>
      T &cast() {
        T *ptr = dynamic_cast<T *>(this);
        if (!ptr) CBANG_THROW("Cannot cast Request to " << type_name<T>());
        return *ptr;
      }

      uint64_t getID() const;

      void setInputHeaders (const HeadersPtr &hdrs) {inputHeaders  = hdrs;}
      void setOutputHeaders(const HeadersPtr &hdrs) {outputHeaders = hdrs;}
      const Headers &getInputHeaders() const {return *inputHeaders;}
      const Headers &getOutputHeaders() const {return *outputHeaders;}
      Headers &getInputHeaders();
      Headers &getOutputHeaders();

      const Event::Buffer &getInputBuffer() const {return inputBuffer;}
      const Event::Buffer &getOutputBuffer() const {return outputBuffer;}
      Event::Buffer &getInputBuffer() {return inputBuffer;}
      Event::Buffer &getOutputBuffer() {return outputBuffer;}

      Method getMethod() const {return method;}
      void setMethod(Method method) {this->method = method;}

      const URI &getURI() const {return uri;}
      void setURI(const URI &uri) {this->uri = uri;}
      std::string getHost() const {return uri.getHost();}

      const Version &getVersion() const {return version;}
      void setVersion(const Version &version) {this->version = version;}

      bool isOk() const;
      Status getResponseCode() const {return responseCode;}
      void setResponseCodeLine(const std::string &s) {responseCodeLine = s;}
      const std::string &getResponseCodeLine() const {return responseCodeLine;}
      std::string getResponseLine() const;
      std::string getRequestLine() const;
      bool logResponseErrors() const;

      bool hasConnection() const {return connection.isSet();}
      const SmartPointer<Conn>::Weak &getConnection() const {return connection;}
      void setConnection(const SmartPointer<Conn> &con) {connection = con;}
      bool isConnected() const;
      Event::ConnectionError getConnectionError() const {return connError;}
      void setConnectionError(Event::ConnectionError err) {connError = err;}

      const SmartPointer<Session> &getSession() const {return session;}
      void setSession(const SmartPointer<Session> &session)
      {this->session = session;}
      std::string
      getSessionID(const std::string &cookie = "sid",
                   const std::string &header = "Authorization") const;

      const std::string &getUser() const;
      void setUser(const std::string &user);

      virtual bool isWebsocket() const {return false;}
      bool isIncoming() const;
      bool isChunked() const {return chunked;}
      bool isReplying() const {return replying;}

      uint64_t getBytesRead() const {return bytesRead;}
      uint64_t getBytesWritten() const {return bytesWritten;}

      bool isSecure() const;
      SSL getSSL() const;

      const JSON::ValuePtr &getArgs() const {return args;}
      void parseJSONArgs();
      void parseQueryArgs();
      const JSON::ValuePtr &parseArgs();

      SockAddr getClientAddr() const;

      bool inHas(const std::string &name) const;
      std::string inFind(const std::string &name) const;
      std::string inGet(const std::string &name) const;
      void inSet(const std::string &name, const std::string &value);
      void inRemove(const std::string &name);

      bool outHas(const std::string &name) const;
      std::string outFind(const std::string &name) const;
      std::string outGet(const std::string &name) const;
      void outSet(const std::string &name, const std::string &value);
      void outRemove(const std::string &name);

      bool isPersistent() const;
      void setPersistent(bool x);
      virtual void setCache(uint32_t age);

      bool hasContentType() const;
      std::string getContentType() const;
      bool isJSONContentType() const;
      void setContentType(const std::string &contentType);
      void guessContentType();

      void outSetContentEncoding(Compression compression);
      Compression getRequestedCompression() const;

      bool hasCookie(const std::string &name) const;
      std::string findCookie(const std::string &name) const;
      std::string getCookie(const std::string &name) const;
      void setCookie(const std::string &name, const std::string &value,
                     const std::string &domain = std::string(),
                     const std::string &path = std::string(),
                     uint64_t expires = 0, uint64_t maxAge = 0,
                     bool httpOnly = false, bool secure = false,
                     const std::string &sameSite = "Lax");

      std::string getInput() const;
      std::string getOutput() const;

      SmartPointer<JSON::Value> getInputJSON() const;
      void setJSONMessage(const SmartPointer<JSON::Value> &msg)
        {this->msg = msg;}
      const SmartPointer<JSON::Value> &getJSONMessage();
      SmartPointer<JSON::Writer> getJSONWriter();

      SmartPointer<std::istream> getInputStream() const;
      SmartPointer<std::ostream>
      getOutputStream(Compression compression = COMPRESSION_NONE);

      virtual void sendJSONError(Status code, const std::string &message);
      virtual void sendError(Status code, const std::string &message);
      virtual void sendError(Status code);
      virtual void sendError(Status code, const Exception &e);
      virtual void sendError(const Exception &e);
      virtual void sendError(const std::exception &e);

      virtual void send(const Event::Buffer &buf);
      virtual void send(const char *data, unsigned length);
      virtual void send(const char *s);
      virtual void send(const std::string &s);
      virtual void sendFile(const std::string &path);

      virtual void reply(Status::enum_t code = HTTP_OK);
      virtual void reply(const Event::Buffer &buf);
      virtual void reply(const char *data, unsigned length);
      virtual void reply(Status code, const char *data, unsigned length);

      template <typename T>
      void reply(Status code, const T &data) {send(data); reply(code);}

      virtual void startChunked(Status code = HTTP_OK);
      virtual void sendChunk(const Event::Buffer &buf);
      virtual void sendChunk(const char *data, unsigned length);
      virtual SmartPointer<JSON::Writer> getJSONChunkWriter();
      virtual void endChunked();

      virtual void redirect(
        const URI &uri, Status code = HTTP_TEMPORARY_REDIRECT);

      // Callbacks
      virtual bool onContinue() {return true;}
      virtual void onResponse(Event::ConnectionError error);
      virtual void onRequest();
      virtual void onWriteComplete(bool success) {}
      virtual void onComplete() {}

      // Used by Connection
      bool mustHaveBody() const;
      bool mayHaveBody() const;
      bool needsClose() const;

      static Version parseHTTPVersion(const std::string &s);
      void parseResponseLine(const std::string &line);

      virtual void write();

    protected:
      virtual void writeResponse(Event::Buffer &buf);
      virtual void writeRequest(Event::Buffer &buf);
      void writeHeaders(Event::Buffer &buf);
    };

    typedef SmartPointer<Request> RequestPtr;
  }
}

#include "Conn.h"