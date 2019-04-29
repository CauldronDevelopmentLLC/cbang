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

#include "Buffer.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <event2/buffer.h>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#endif

using namespace std;
using namespace cb::Event;


extern "C" {
  void evbuffer_incref_(struct evbuffer *buf);
}


Buffer::Buffer() : evb(evbuffer_new()) {
  if (!evb) THROW("Failed to create event buffer");
}


Buffer::Buffer(evbuffer *evb) : evb(evb) {
  if (evb) evbuffer_incref_(evb);
}


Buffer::Buffer(const char *data, unsigned length) : Buffer() {
  add(data, length);
}


Buffer::Buffer(const char *s) : Buffer() {add(s);}
Buffer::Buffer(const string &s) : Buffer() {add(s);}
Buffer::~Buffer() {if (evb) evbuffer_free(evb);}


Buffer &Buffer::operator=(const Buffer &o) {
  if (evb) evbuffer_free(evb);
  evb = o.evb;
  if (evb) evbuffer_incref_(evb);
  return *this;
}


unsigned Buffer::getLength() const {return evbuffer_get_length(evb);}


const char *Buffer::toCString() const {
  return (const char *)evbuffer_pullup(evb, -1);
}


string Buffer::toString() const {return string(toCString(), getLength());}


string Buffer::hexdump() const {
  return String::hexdump(toCString(), getLength());
}


void Buffer::freeze(bool enable, bool front) {
  if ((enable ? evbuffer_freeze : evbuffer_unfreeze)(evb, front))
    THROW("Failed to " << (enable ? "freeze" : "unfreeze") << " buffer at "
          << (front ? "front" : "back"));
}


void Buffer::clear() {
  freeze(false, true);
  drain(getLength());
}


void Buffer::expand(unsigned length) {
  if (evbuffer_expand(evb, length)) THROW("Failed to expand buffer");
}


char *Buffer::pullup(int length) {
  return (char *)evbuffer_pullup(evb, length);
}


unsigned Buffer::copy(char *data, unsigned length) {
  ev_ssize_t ret = evbuffer_copyout(evb, data, length);
  if (ret < 0) THROW("Failed to copy from buffer");
  return (unsigned)ret;
}


unsigned Buffer::copy(ostream &stream, unsigned length) {
  unsigned total = 0;
  char buffer[4096];

  while (0 < length) {
    unsigned size = copy(buffer, min(length, (unsigned)4096));
    stream.write(buffer, size);
    length -= size;
    total += size;
  }

  return total;
}


unsigned Buffer::copy(ostream &stream) {return copy(stream, getLength());}


void Buffer::drain(unsigned length) {
  if (length && evbuffer_drain(evb, length))
    THROW("Buffer draining " << length << " bytes failed");
}


unsigned Buffer::remove(char *data, unsigned length) {
  ev_ssize_t ret = evbuffer_remove(evb, data, length);
  if (ret < 0) THROW("Failed to remove data from buffer");
  return (unsigned)ret;
}


unsigned Buffer::remove(Buffer &buf, unsigned length) {
  ev_ssize_t ret = evbuffer_remove_buffer(evb, buf.evb, length);
  if (ret < 0) THROW("Failed to remove data from buffer");
  return (unsigned)ret;
}


unsigned Buffer::remove(ostream &stream, unsigned length) {
  unsigned total = 0;
  char buffer[4096];

  while (!stream.fail() && 0 < length) {
    unsigned size = remove(buffer, min(length, (unsigned)4096));
    stream.write(buffer, size);
    length -= size;
    total += size;
  }

  return total;
}


unsigned Buffer::remove(ostream &stream) {return remove(stream, getLength());}


string Buffer::readLine(unsigned maxLength, eol_t eol) {
  // TODO Don't read more then maxLength
  size_t length = 0;
  SmartPointer<char>::Malloc line =
    evbuffer_readln(evb, &length, (evbuffer_eol_style)eol);
  if (line.isNull()) THROW("Failed to read line from buffer");
  return string(line.get(), length);
}


void Buffer::add(const Buffer &buf) {
  if (evbuffer_add_buffer(evb, buf.getBuffer())) THROW("Add buffer failed");
}


void Buffer::addRef(const Buffer &buf) {
  if (evbuffer_add_buffer_reference(evb, buf.getBuffer()))
    THROW("Add buffer reference failed");
}


void Buffer::add(const char *data, unsigned length) {
  if (evbuffer_add(evb, data, length)) THROW("Buffer add failed");
}


void Buffer::add(const char *s) {add(s, strlen(s));}
void Buffer::add(const string &s) {add(CBANG_CPP_TO_C_STR(s), s.length());}


void Buffer::addFile(const string &path) {
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) THROW("Failed to open file " << path);

  struct stat buf;
  if (fstat(fd, &buf)) THROW("Failed to get file size " << path);

  if (evbuffer_add_file(evb, fd, 0, buf.st_size))
    THROW("Failed to add file to buffer: " << path);
}


void Buffer::prepend(const Buffer &buf) {
  if (evbuffer_prepend_buffer(evb, buf.getBuffer()))
    THROW("Prepend buffer failed");
}


void Buffer::prepend(const char *data, unsigned length) {
  if (evbuffer_prepend(evb, data, length)) THROW("Buffer prepend failed");
}


void Buffer::prepend(const char *s) {prepend(s, strlen(s));}


void Buffer::prepend(const string &s) {
  prepend(CBANG_CPP_TO_C_STR(s), s.length());
}
