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

#include "ConnOut.h"
#include "Client.h"

#include <cbang/Catch.h>
#include <cbang/net/Socket.h>
#include <cbang/log/Logger.h>

using namespace cb::HTTP;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "CON" << getID() << ':'


ConnOut::ConnOut(Event::Base &base) : Conn(base) {}


void ConnOut::writeRequest(
  const SmartPointer<Request> &req, Event::Buffer buffer, bool hasMore,
  function<void (bool)> cb) {
  if (!isConnected()) THROW("Cannot write request, not connected");

  LOG_DEBUG(4, CBANG_FUNC << "() length=" << buffer.getLength());
  LOG_DEBUG(4, "Sending: " << buffer.toString());

  checkActive(req);

  auto cb2 =
    [this, req, hasMore, cb] (bool success) {
      if (cb) TRY_CATCH_ERROR(cb(success));
      if (!success) return fail(CONN_ERR_EOF, "Failed to write request");
      if (hasMore) return; // Still writing

      readHeader(req);
   };

  write(cb2, buffer);
}


void ConnOut::makeRequest(const SmartPointer<Request> &req) {
  push(req);
  if (getNumRequests() == 1) dispatch();
}


void ConnOut::fail(Event::ConnectionError err, const string &msg) {
  LOG_DEBUG(3, msg);

  auto requests = this->requests;
  this->requests.clear();

  while (requests.size()) {
    TRY_CATCH_ERROR(requests.front()->onResponse(err));
    requests.pop_front();
  }

  close();
}


void ConnOut::readHeader(const SmartPointer<Request> &req) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  auto cb =
    [this, req] (bool success) {
      LOG_DEBUG(4, CBANG_FUNC << "() success=" << success
                << " length=" << input.getLength());

      if (maxHeaderSize && maxHeaderSize <= input.getLength())
        return fail(CONN_ERR_BAD_RESPONSE, "Header too large");

      if (!success) return fail(CONN_ERR_EOF, "Failed to read response header");

      readBody(req);
    };

  // Read until end of header
  read(cb, input, getMaxHeaderSize(), "\r\n\r\n");
}


void ConnOut::readBody(const SmartPointer<Request> &req) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  // Read first line
  try {
    string line;
    input.readLine(line, maxHeaderSize);
    req->parseResponseLine(line);

  } catch (const Exception &e) {
    return fail(CONN_ERR_BAD_RESPONSE, e.getMessage());
  }

  // Read header block
  if (!req->getInputHeaders().parse(input))
    return fail(CONN_ERR_BAD_RESPONSE, "Incomplete headers");

  LOG_DEBUG(4, req->getInputHeaders());

  // Headers callback
  req->onHeaders();

  if (req->getResponseCode() == 100) return readHeader(req);
  if (!req->mustHaveBody()) return process(req);

  // Handle chunked data
  string xferEnc = String::toLower(req->inFind("Transfer-Encoding"));
  if (xferEnc == "chunked") {
    auto cb =
      [this, req] (bool success) {
        if (success) process(req);
        else fail(CONN_ERR_BAD_RESPONSE, "Failed to read chunked body");
      };

    return readChunks(req, cb);
  }

  // Compute body length
  int contentLength = -1;
  string contentLengthStr = req->inFind("Content-Length");

  if (contentLengthStr.empty()) {
    // Check for Connection: close
    string connection = String::toLower(req->inFind("Connection"));

    if (connection != "close")
      // Bad combination, cannot tell when communication should end
      fail(CONN_ERR_BAD_RESPONSE,
           "No Content-Length but peer wants to keep connection open");

  } else {
    try {
      contentLength = String::parseU32(contentLengthStr);
    } catch (const Exception &e) {
      return fail(CONN_ERR_BAD_RESPONSE, "Invalid Content-Length");
    }

    if (maxBodySize && maxBodySize < (unsigned)contentLength)
      return fail(CONN_ERR_BAD_RESPONSE, "Response body too large");

    // Allocate space
    uint32_t bytes = input.getLength();
    if (bytes < (unsigned)contentLength) input.expand(contentLength - bytes);
  }

  // Compute read size
  uint32_t readSize = numeric_limits<uint32_t>::max();
  if (0 <= contentLength) readSize = contentLength;
  else if (maxBodySize) readSize = maxBodySize;

  // Read body
  auto cb =
    [this, req, contentLength] (bool success) {
      if (0 <= contentLength && input.getLength() < (unsigned)contentLength)
        return fail(CONN_ERR_EOF, "Failed to read response body");

      process(req);
    };

  read(cb, input, readSize);
}


void ConnOut::process(const SmartPointer<Request> &req) {
  LOG_DEBUG(4, __func__ << "()");

  // Remove request
  pop();

  try {
    req->getInputBuffer().add(input);

    // Callback
    req->onResponse(CONN_ERR_OK);

    // If not closing send next request
    if (!req->needsClose()) return dispatch();
  } CATCH_ERROR;

  close();
}


void ConnOut::dispatch() {
  if (getNumRequests()) getRequest()->write();
}
