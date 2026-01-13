/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include "StreamBuf.h"

#include <cbang/log/Logger.h>
#include <cbang/os/SysError.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>
#include <io.h>

#define mode_t int
#define ssize_t SSIZE_T

#else
#include <unistd.h>
#endif

using namespace std;
using namespace cb;


#define IS_SET(x, y) (((x) & (y)) == (y))


namespace {
  int openModeToFlags(ios::openmode mode) {
    int flags = 0;

    if (IS_SET(mode, ios::in | ios::out)) flags |= O_RDWR;
    else if (IS_SET(mode, ios::in)) flags |= O_RDONLY;
    else if (IS_SET(mode, ios::out)) {
      flags |= O_WRONLY;
      if (!(IS_SET(mode, ios::ate) || IS_SET(mode, ios::app)))
        flags |= O_TRUNC;
    }

    if (IS_SET(mode, ios::out)) {
      if (IS_SET(mode, ios::trunc)) flags |= O_TRUNC;
      if (IS_SET(mode, ios::app))   flags |= O_APPEND;
      flags |= O_CREAT; // The default for ios::out
    }

#ifdef _WIN32
    flags |= O_BINARY;    // Always use binary mode
    flags |= O_NOINHERIT; // Don't inherit handles by default
#else
    flags |= O_CLOEXEC;   // Close on exec
#endif

#ifdef _LARGEFILE64_SOURCE
    flags |= O_LARGEFILE;
#endif

    return flags;
  }


  int seekDirToWhence(ios::seekdir way) {
    switch (way) {
    case ios::beg: return SEEK_SET;
    case ios::cur: return SEEK_CUR;
    case ios::end: return SEEK_END;
    default: THROW("Invalid seek direction: " << way);
    }
  }
}


StreamBuf::StreamBuf(const string &path, ios::openmode mode, int perm) {
#ifdef _WIN32
  perm &= 0600; // Windows only understands these permissions
#endif

  fd = ::open(path.c_str(), openModeToFlags(mode), perm);
  if (fd == -1) THROW("Failed to open file: " << SysError());
}


void StreamBuf::close() {
  sync();
  if (fd != -1 && ::close(fd))
    LOG_ERROR("close(" << fd << ") failed: " << SysError());
  fd = -1;
}


StreamBuf::int_type StreamBuf::underflow() {
  if (fd < 0) return traits_type::eof();

  if (readBuf.isSet()) {
    if (gptr()) *readBuf = *(gptr() - 1);
  } else readBuf = new char[bufferSize];

  auto bytes = ::read(fd, readBuf.get() + 1, bufferSize - 1);
  if (bytes <= 0) return traits_type::eof();

  setg(readBuf.get(), readBuf.get() + 1, readBuf.get() + 1 + bytes);

  return traits_type::to_int_type(*gptr());
}


StreamBuf::int_type StreamBuf::overflow(int_type c) {
  if (fd < 0) return traits_type::eof();

  if (writeBuf.isNull()) writeBuf = new char[bufferSize];
  if (c == traits_type::eof() || sync() == -1) return traits_type::eof();
  *pptr() = traits_type::to_char_type(c);
  pbump(1);

  return c;
}


int StreamBuf::sync() {
  if (fd < 0 || writeBuf.isNull()) return 0;

  auto p = pbase();

  while (p < pptr()) {
    auto bytes = ::write(fd, p, pptr() - p);
    if (bytes <= 0) return -1;
    p += bytes;
  }

  setp(writeBuf.get(), writeBuf.get() + bufferSize);

  return 0;
}


streampos StreamBuf::seekoff(streamoff off, ios::seekdir way,
                             ios::openmode which) {
  if (fd < 0) return -1;

  sync();
  if (readBuf.isSet()) setg(0, 0, 0);

  return lseek(fd, off, seekDirToWhence(way));
}


streampos StreamBuf::seekpos(streampos sp, ios::openmode which) {
  return seekoff(sp, ios::beg, which);
}
