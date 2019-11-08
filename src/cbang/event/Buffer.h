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

#include "Socket.h"

#include <string>
#include <ostream>

#include <functional>
#include <vector>

struct evbuffer;
struct evbuffer_cb_entry;

#ifdef _WIN32
struct evbuffer_iovec;
typedef struct evbuffer_iovec iovec;

#else
struct iovec;
#endif


namespace cb {
  namespace Event {
    class Buffer {
    public:
      typedef std::function<void (int added, int deleted, int orig)> callback_t;

    protected:
      evbuffer *evb;
      callback_t cb;
      evbuffer_cb_entry *cbEntry = 0;

    public:
      Buffer();
      Buffer(evbuffer *evb);
      Buffer(const Buffer &o) : Buffer(o.evb) {}
      Buffer(const char *data, unsigned length);
      Buffer(const char *s);
      Buffer(const std::string &s);
      ~Buffer();

      Buffer &operator=(const Buffer &o);

      evbuffer *getBuffer() const {return evb;}

      unsigned getLength() const;

      const char *toCString() const;
      std::string toString() const;
      std::string hexdump() const;

      callback_t getCallback() const {return cb;}
      void setCallback(const callback_t &cb, unsigned flags = 0);

      void setFlags(uint64_t flags);
      void freeze(bool enable, bool front);
      void clear();
      void expand(unsigned length);
      char *pullup(int length = -1);

      unsigned copy(char *data, unsigned length);
      unsigned copy(std::ostream &stream, unsigned length);
      unsigned copy(std::ostream &stream);
      void drain(unsigned length);
      unsigned remove(Buffer &buf, unsigned length);
      unsigned remove(char *data, unsigned length);
      unsigned remove(std::ostream &stream, unsigned length);
      unsigned remove(std::ostream &stream);

      typedef enum {
        EOL_ANY,
        EOL_CRLF,
        EOL_CRLF_STRICT,
        EOL_LF,
        EOL_NULL
      } eol_t;

      int read(socket_t fd, int size);
      int write(socket_t fd, int size = -1);

      void peek(unsigned bytes, std::vector<iovec> &space);
      void reserve(unsigned bytes, std::vector<iovec> &space);
      void commit(std::vector<iovec> &space);
      void commit(iovec &space);

      std::string readLine(unsigned maxLength, eol_t eol = EOL_CRLF);

      void add(const Buffer &buf);
      void addRef(const Buffer &buf);
      void add(const char *data, unsigned length);
      void add(const char *s);
      void add(const std::string &s);
      void addFile(const std::string &path);

      void prepend(const Buffer &buf);
      void prepend(const char *data, unsigned length);
      void prepend(const char *s);
      void prepend(const std::string &s);

      void callback(int added, int deleted, int orig);
    };
  }
}
