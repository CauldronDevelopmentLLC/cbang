/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "Conn.h"
#include "Server.h"
#include "Request.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/util/WeakCallback.h>

using namespace cb::HTTP;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "CON" << getID() << ':'


Conn::Conn(Event::Base &base) : Event::Connection(base) {}
Conn::~Conn() {}


void Conn::readChunks(
  const SmartPointer<Request> &req, function<void (bool)> cb) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  Event::Transfer::cb_t readCB =
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

  read(WeakCall(this, readCB), input, 1024, "\r\n");
}


void Conn::readChunk(
  const SmartPointer<Request> &req, uint32_t size, function<void (bool)> cb) {
  LOG_DEBUG(4, CBANG_FUNC << "() size=" << size);

  if (!size) return readChunkTrailer(req, cb);

  // Update body size
  if (maxBodySize && maxBodySize < size + req->getInputBuffer().getLength()) {
    LOG_WARNING("Chunked body too large");
    if (cb) cb(false);
    return;
  }

  // Read chunk
  Event::Transfer::cb_t readCB =
    [this, req, size, cb] (bool success) mutable {
      if (success && size <= input.getLength()) {
        input.remove(req->getInputBuffer(), size);
        input.drain(2); // Remove CRLF
        readChunks(req, cb); // Next chunk

      } else if (cb) cb(false);
    };

  read(WeakCall(this, readCB), input, size + 2);
}


void Conn::readChunkTrailer(
  const SmartPointer<Request> &req, function<void (bool)> cb) {
  LOG_DEBUG(4, CBANG_FUNC << "()");

  Event::Transfer::cb_t readCB =
    [this, req, cb] (bool success) {
      // No header lines
      if (input.indexOf("\r\n") == 0) {
        if (cb) cb(true);
        return;
      }

      // Has header lines in trailer
      Event::Transfer::cb_t readCB =
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

      read(WeakCall(this, readCB), input, maxHeaderSize,
        "\r\n\r\n");
    };

  read(WeakCall(this, readCB), input, maxHeaderSize, "\r\n");
}


void Conn::checkActive(const SmartPointer<Request> &req) {
  if (requests.empty() || requests.front() != req)
    THROW("Not the active request");
}


const SmartPointer<Request> &Conn::getRequest() {
  if (requests.empty()) THROW("No requests");
  return requests.front();
}


void Conn::push(const SmartPointer<Request> &req) {
  requests.push_back(req);
}


void Conn::pop() {
  if (requests.empty()) THROW("No requests");
  TRY_CATCH_ERROR(requests.front()->onComplete());
  requests.pop_front();
}


void Conn::close() {
  auto self = SmartPtr(this);
  while (!requests.empty()) pop();
  Event::Connection::close();
}
