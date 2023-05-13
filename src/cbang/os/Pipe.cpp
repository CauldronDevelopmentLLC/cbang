/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "Pipe.h"
#include "SysError.h"

#include <cbang/Exception.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else // _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

#include <boost/version.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#if BOOST_VERSION < 104400
#define BOOST_CLOSE_HANDLE true
#else
#define BOOST_CLOSE_HANDLE io::close_handle
#endif

namespace io = boost::iostreams;

using namespace cb;


Pipe::Pipe(bool toChild) : toChild(toChild) {
  handles[0] = handles[1] = -1;
  closeHandles[0] = closeHandles[1] = false;
}


Pipe::handle_t Pipe::getHandle(bool childEnd) const {
  return handles[(toChild ^ childEnd) ? 1 : 0];
}


void Pipe::closeHandle(bool childEnd) {
  int end = (toChild ^ childEnd) ? 1 : 0;

  if (closeHandles[end]) {
#ifdef _WIN32
    CloseHandle(handles[end]);
#else
    ::close(handles[end]);
#endif

    closeHandles[end] = false;
  }
}


void Pipe::setBlocking(bool blocking, bool childEnd) {
  handle_t handle = getHandle(childEnd);

  if (handle == -1) THROW("Pipe handle not open");

#ifdef _WIN32
  if (SetNamedPipeHandleState
      (handle, blocking ? PIPE_WAIT : PIPE_NOWAIT, 0, 0)) return;

#else
  int opts = fcntl(handle, F_GETFL);
  if (0 <= opts) {
    if (blocking) opts &= ~O_NONBLOCK;
    else opts |= O_NONBLOCK;

    if (!fcntl(handle, F_SETFL, opts)) return;
  }
#endif

  THROW("Failed to set pipe non-blocking: " << SysError());
}


void Pipe::setSize(int size, bool childEnd) {
  if (getHandle(childEnd) == -1) THROW("Pipe handle not open");

#ifdef _WIN32
  THROW("Pipe set size is not supported on this platform");

#else
  if (fcntl(getHandle(childEnd), F_SETPIPE_SZ, size) == -1)
    THROW("Failed to set pipe size to " << size << ": " << SysError());
#endif
}


void Pipe::create() {
  if (getChildHandle() != -1 || getParentHandle() != -1)
    THROW("Pipe already created");

#ifdef _WIN32
  // Setup security attributes for pipe inheritance
  SECURITY_ATTRIBUTES sAttrs;
  memset(&sAttrs, 0, sizeof(SECURITY_ATTRIBUTES));
  sAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
  sAttrs.bInheritHandle = TRUE;
  sAttrs.lpSecurityDescriptor = 0;

  if (!CreatePipe(&handles[0], &handles[1], &sAttrs, 0))
    THROW("Failed to create pipe: " << SysError());

  // Don't inherit other handle
  if (!SetHandleInformation(handles[toChild ? 1 : 0],
                            HANDLE_FLAG_INHERIT, 0))
    THROW("Failed to clear pipe inherit flag: " << SysError());

#else
  if (pipe(handles)) THROW("Failed to create pipe: " << SysError());
#endif

  closeHandles[0] = closeHandles[1] = true;
}


void Pipe::close() {
  closeParentHandle();
  closeChildHandle();
}


const SmartPointer<std::iostream> &Pipe::getStream() {
  if (stream.isNull()) {
    if (getParentHandle() == -1) THROW("Pipe not open");
    typedef io::stream<io::file_descriptor> stream_t;
    stream = new stream_t(getParentHandle(), BOOST_CLOSE_HANDLE);
    closeHandles[toChild ? 1 : 0] = false; // Don't close this handle in close()
  }

  return stream;
}


void Pipe::closeStream() {stream.release();}


void Pipe::inChildProc(int target) {
#ifndef _WIN32
  closeParentHandle();

  // Move to target
  if (target != -1 && dup2(getChildHandle(), target) != target)
    perror("Moving file descriptor");
#endif
}
