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

#include "HTTPConnIn.h"
#include "HTTPServer.h"
#include "Buffer.h"
#include "Request.h"

#include <cbang/log/Logger.h>

using namespace cb::Event;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX << "CON" << getID() << ':'


HTTPConnIn::HTTPConnIn(HTTPServer &server) :
  HTTPConn(server.getBase()), server(server) {}


void HTTPConnIn::writeRequest(const SmartPointer<Request> &req, Buffer buffer,
                              bool hasMore) {
  LOG_DEBUG(4, CBANG_FUNC << "() length=" << buffer.getLength() << " hasMore="
            << hasMore);

  checkActive(req);

  if (getStats().isSet()) getStats()->event(req->getResponseCode().toString());

  auto cb =
    [this, req, hasMore] (bool success) {
      LOG_DEBUG(6, "Response " << (success ? "successful" : "failed")
                << " hasMore=" << hasMore << " persistent="
                << req->isPersistent() << " numReqs=" << getNumRequests());

      // Handle write failure
      if (!success) return close();
      if (hasMore) return; // Still writing

      if (getNumRequests()) pop();

      // Free connection if not persistent
      if (!req->isPersistent()) return close();

      // Handle another request
      if (getNumRequests()) processRequest(getRequest());
      else readHeader();
    };

  write(cb, buffer);
}


void HTTPConnIn::readHeader() {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  auto cb =
    [this] (bool success) {
      if (maxHeaderSize && maxHeaderSize <= input.getLength())
        error(HTTP_BAD_REQUEST, "Header too large");

      else if (!success) close();
      else processHeader();
    };

  // Read until end of header
  read(cb, input, getMaxHeaderSize(), "\r\n\r\n");
}


void HTTPConnIn::processHeader() {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  // Read first line
  RequestMethod method;
  URI uri;
  Version version;
  try {
    string line;
    input.readLine(line, maxHeaderSize);
    vector<string> parts;
    String::tokenize(line, parts, " ");

    if (parts.size() != 3)
      THROW("Invalid request line: " << String::escapeC(line));

    method = RequestMethod::parse(parts[0]);
    uri = parts[1];
    version = Request::parseHTTPVersion(parts[2]);

  } catch (const Exception &e) {
    return error(HTTP_BAD_REQUEST, e.getMessage());
  }

  // Create new request
  auto req = server.createRequest(method, uri, version);
  req->setConnection(this);

  // Read header block
  if (!req->getInputHeaders().parse(input))
    return error(HTTP_BAD_REQUEST, "Incomplete headers");

  // Headers callback
  req->onHeaders();

  // If this is a request without a body, then we are done
  if (!req->mayHaveBody()) return addRequest(req);

  // Handle 100 HTTP continue
  if (Version(1, 1) <= version) {
    string expect = String::toLower(req->inFind("Expect"));

    if (!expect.empty()) {
      if (expect == "100-continue" && req->onContinue()) {
        string line = "HTTP/" + version.toString() + " 100 Continue\r\n\r\n";

        auto cb =
          [this, req] (bool success) {
            if (success) checkChunked(req);
            else error(HTTP_BAD_REQUEST, "Failed to send continue");
          };

        return write(cb, line);

      } else return error(HTTP_EXPECTATION_FAILED, "Cannot continue");
    }
  }

  checkChunked(req);
}


void HTTPConnIn::checkChunked(const SmartPointer<Request> &req) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  // Handle chunked data
  string xferEnc = String::toLower(req->inFind("Transfer-Encoding"));
  if (xferEnc == "chunked") {
    auto cb =
      [this, req] (bool success) {
        if (success) addRequest(req);
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
  if (!contentLength) return addRequest(req);

  if (maxBodySize && maxBodySize < contentLength)
    return error(HTTP_REQUEST_ENTITY_TOO_LARGE, "Body too large");

  // Allocate space
  uint32_t bytes = input.getLength();
  if (bytes < contentLength) input.expand(contentLength - bytes);

  // Read body
  auto cb =
    [this, req, contentLength] (bool success) {
      if (input.getLength() < contentLength) {
        LOG_DEBUG(3, "Incomplete request body input=" << input.getLength()
          << " ContentLength=" << contentLength);
        return close();
      }

      if (contentLength) input.remove(req->getInputBuffer(), contentLength);
      addRequest(req);
    };

  read(cb, input, contentLength);
}


void HTTPConnIn::processRequest(const SmartPointer<Request> &req) {
  LOG_INFO(1, "< " << getPeer() << ' ' << req->getRequestLine());
  LOG_DEBUG(5, req->getInputHeaders() << '\n');
  LOG_DEBUG(6, input.hexdump() << '\n');

  server.dispatch(req);
}


void HTTPConnIn::addRequest(const SmartPointer<Request> &req) {
  push(req);
  if (getNumRequests() == 1) processRequest(req);
  // TODO Should read the next request in parallel if persistent connection
}


void HTTPConnIn::error(HTTPStatus code, const std::string &message) {
  LOG_DEBUG(3, "Error: " << code << ": " << message);
  if (!getNumRequests()) close();
  else getRequest()->sendError(code, message);
}
