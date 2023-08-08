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

#include "HTTPConn.h"
#include "HTTPServer.h"
#include "Request.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>

using namespace cb::Event;
using namespace cb;
using namespace std;

#define CBANG_LOG_PREFIX "CON" << getID() << ':'


HTTPConn::HTTPConn(cb::Event::Base &base) : Connection(base) {}


void HTTPConn::readChunks(const SmartPointer<Request> &req,
                          function<void (bool)> cb) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  auto readCB =
    [this, req, cb] (bool success) {
      if (success) {
        try {
          // Parse size
          string line;
          input.readLine(line, 1024);
          size_t end = line.find(';');
          uint32_t size = String::parseU32("0x" + line.substr(0, end));

          return readChunk(req, size, cb);
        } CATCH_ERROR;
      }

      if (cb) cb(false);
    };

  read(readCB, input, 1024, "\r\n");
}


void HTTPConn::readChunk(const SmartPointer<Request> &req, uint32_t size,
                         function<void (bool)> cb) {
  LOG_DEBUG(4, CBANG_FUNC << "() size=" << size);

  if (!size) return readChunkTrailer(req, cb);

  // Update body size
  if (maxBodySize && maxBodySize < size + req->getInputBuffer().getLength()) {
    LOG_WARNING("Chunked body too large");
    if (cb) cb(false);
    return;
  }

  // Read chunk
  auto readCB =
    [this, req, size, cb] (bool success) mutable {
      if (success && size <= input.getLength()) {
        input.remove(req->getInputBuffer(), size);
        input.drain(2); // Remove CRLF
        readChunks(req, cb); // Next chunk

      } else if (cb) cb(false);
    };

  read(readCB, input, size + 2);
}


void HTTPConn::readChunkTrailer(const SmartPointer<Request> &req,
                                function<void (bool)> cb) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  auto readCB =
    [this, req, cb] (bool success) {
      // No header lines
      if (input.indexOf("\r\n") == 0) {
        if (cb) cb(true);
        return;
      }

      // Has header lines in trailer
      auto readCB =
        [this, req, cb] (bool success) {
          try {
            if (req->getInputHeaders().parse(input)) {
              if (cb) cb(true);
              return;
            }
          } CATCH_ERROR;

          LOG_WARNING("Incomplete chunk trailer headers");
          if (cb) cb(false);
        };

      read(readCB, input, maxHeaderSize, "\r\n\r\n");
    };

  read(readCB, input, maxHeaderSize, "\r\n");
}


void HTTPConn::checkActive(const SmartPointer<Request> &req) {
  if (requests.empty() || requests.front() != req)
    THROW("Not the active request");
}


const SmartPointer<Request> &HTTPConn::getRequest() {
  if (requests.empty()) THROW("No requests");
  return requests.front();
}


void HTTPConn::push(const SmartPointer<Request> &req) {
  requests.push_back(req);
}


void HTTPConn::pop() {
  if (requests.empty()) THROW("No requests");
  requests.front()->onComplete();
  requests.pop_front();
}


void HTTPConn::close() {
  SmartPointer<HTTPConn> self(this); // Make sure we don't get deallocated
  Connection::close();
  // Must be after close so all transactions have cleared
  while (!requests.empty()) pop();
}
