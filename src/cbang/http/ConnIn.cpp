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

#include "ConnIn.h"
#include "Server.h"
#include "Request.h"

#include <cbang/Catch.h>
#include <cbang/event/Buffer.h>
#include <cbang/log/Logger.h>
#include <cbang/ws/Websocket.h>
#include <cbang/util/WeakCallback.h>

using namespace cb::HTTP;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "CON" << getID() << ':'


ConnIn::ConnIn(Server &server) :
  Conn(server.getBase()), server(server) {}


void ConnIn::writeRequest(
  const SmartPointer<Request> &req, Event::Buffer buffer,
  bool continueProcessing, function<void (bool)> cb) {
  LOG_DEBUG(4, CBANG_FUNC << "() length=" << buffer.getLength() << " hasMore="
            << continueProcessing);

  checkActive(req);

  if (getStats().isSet()) getStats()->event(req->getResponseCode().toString());

  Event::Transfer::cb_t cb2 =
    [this, req, continueProcessing, cb] (bool success) {
      LOG_DEBUG(6, "Response " << (success ? "successful" : "failed")
                << " continueProcessing=" << continueProcessing
                << " persistent=" << req->isPersistent()
                << " numReqs=" << getNumRequests());

      if (cb) TRY_CATCH_ERROR(cb(success));

      // Handle write failure
      if (!success) return close();
      if (!continueProcessing) return;

      if (getNumRequests()) pop();

      // Free connection if not persistent
      if (!req->isPersistent()) return close();

      // Handle another request
      if (getNumRequests()) processRequest(getRequest());
      else readHeader();
    };

  write(WeakCall(this, cb2), buffer);
}


void ConnIn::readHeader() {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  Event::Transfer::cb_t cb =
    [this] (bool success) {
      if (maxHeaderSize && maxHeaderSize <= input.getLength())
        error(HTTP_BAD_REQUEST, "Header too large");

      else if (!success) close();
      else processHeader();
    };

  // Read until end of header
  read(WeakCall(this, cb), input, getMaxHeaderSize(), "\r\n\r\n");
}


void ConnIn::processHeader() {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  // Read first line
  Method method;
  URI uri;
  Version version;
  try {
    string line;
    input.readLine(line, maxHeaderSize);
    vector<string> parts;
    String::tokenize(line, parts, " ");

    if (parts.size() != 3)
      THROW("Invalid request line: " << String::escapeC(line));

    method = Method::parse(parts[0]);
    uri = parts[1];
    version = Request::parseHTTPVersion(parts[2]);

  } catch (const Exception &e) {
    return error(HTTP_BAD_REQUEST, e.getMessage());
  }

  // Read header block
  auto hdrs = SmartPtr(new Headers);
  if (!hdrs->parse(input)) return error(HTTP_BAD_REQUEST, "Incomplete headers");

  // Create new request (Don't create circular dependency)
  auto req = server.createRequest({this, method, uri, version, hdrs});
  push(req);

  // If this is a request without a body, then we are done
  if (!req->mayHaveBody()) return processIfNext(req);
  else if (req->inHas("Upgrade")) error(HTTP_BAD_REQUEST, "Cannot upgrade");

  // Handle 100 HTTP continue
  if (Version(1, 1) <= version) {
    string expect = String::toLower(req->inFind("Expect"));

    if (!expect.empty()) {
      if (expect == "100-continue") {
        string line = "HTTP/" + version.toString() + " 100 Continue\r\n\r\n";

        Event::Transfer::cb_t cb =
          [this, req] (bool success) {
            if (success) checkChunked(req);
            else error(HTTP_BAD_REQUEST, "Failed to send continue");
          };

        write(WeakCall(this, cb), line);
        return;

      } else return error(HTTP_EXPECTATION_FAILED, "Cannot continue");
    }
  }

  checkChunked(req);
}


void ConnIn::checkChunked(const SmartPointer<Request> &req) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  // Handle chunked data
  string xferEnc = String::toLower(req->inFind("Transfer-Encoding"));
  if (xferEnc == "chunked") {
    auto cb =
      [this, req] (bool success) {
        if (success) processIfNext(req);
        else {
          LOG_DEBUG(3, "Incomplete chunked request body");
          return close();
        }
      };

    return readChunks(req, cb);
  }

  // Parse Content-Length
  unsigned contentLength = 0;
  try {
    contentLength = String::parseU32(req->inFind("Content-Length"));
  } catch (const Exception &e) {
    return error(HTTP_BAD_REQUEST, "Invalid Content-Length");
  }

  // Non-chunked request /wo Content-Length has no body
  if (!contentLength) return processIfNext(req);

  if (maxBodySize && maxBodySize < contentLength)
    return error(HTTP_REQUEST_ENTITY_TOO_LARGE, "Body too large");

  // Allocate space
  uint32_t bytes = input.getLength();
  if (bytes < contentLength) input.expand(contentLength - bytes);

  // Read body
  Event::Transfer::cb_t cb =
    [this, req, contentLength] (bool success) {
      if (input.getLength() < contentLength) {
        LOG_DEBUG(3, "Incomplete request body input=" << input.getLength()
          << " ContentLength=" << contentLength);
        return close();
      }

      if (contentLength) input.remove(req->getInputBuffer(), contentLength);
      processIfNext(req);
    };

  read(WeakCall(this, cb), input, contentLength);
}


void ConnIn::processRequest(const SmartPointer<Request> &req) {
  TRY_CATCH_ERROR(req->onRequest());
  server.dispatch(*req);
}


void ConnIn::processIfNext(const SmartPointer<Request> &req) {
  if (getNumRequests() && getRequest() == req) processRequest(req);
  // TODO Should read the next request in parallel if persistent connection
}


void ConnIn::error(Status code, const string &message) {
  if (!code) {
    LOG_ERROR(message);
    code = HTTP_INTERNAL_SERVER_ERROR;
  }

  LOG_DEBUG(3, "Error: " << code << ": " << message);

  if (!getNumRequests()) close();
  else getRequest()->sendError(code, message);
}
