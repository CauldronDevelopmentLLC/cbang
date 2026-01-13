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

#include "Pipe.h"
#include "SysError.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>
#include <cbang/boost/IOStreams.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else // _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

#include <cbang/boost/StartInclude.h>
#include <boost/version.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cbang/boost/StartInclude.h>

#if BOOST_VERSION < 104400
#define BOOST_CLOSE_HANDLE true
#else
#define BOOST_CLOSE_HANDLE io::close_handle
#endif

using namespace cb;


const PipeEnd::handle_t PipeEnd::INVALID = (handle_t)-1;


void PipeEnd::close() {
  if (!isOpen()) return;

#ifdef _WIN32
  if (!CloseHandle(handle))
#else
  if (::close(handle))
#endif
    LOG_ERROR("Closing pipe " << handle << ": " << SysError());

  handle = INVALID;
}


void PipeEnd::setBlocking(bool blocking) {
  if (!isOpen()) THROW("Pipe end not open");

#ifdef _WIN32
  DWORD mode = blocking ? PIPE_WAIT : PIPE_NOWAIT;
  if (SetNamedPipeHandleState(handle, &mode, 0, 0)) return;

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


void PipeEnd::setInheritFlag(bool inherit) {
  if (!isOpen()) THROW("Pipe end not open");

#ifdef _WIN32

  DWORD flags = inherit ? HANDLE_FLAG_INHERIT : 0;
  if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, flags))
    THROW("Failed to set pipe inherit flag: " << SysError());

#else
  THROW(CBANG_FUNC << "() not supported");
#endif
}


void PipeEnd::setSize(int size) {
  if (!isOpen()) THROW("Pipe end not open");

#ifdef F_SETPIPE_SZ
  if (fcntl(handle, F_SETPIPE_SZ, size) == -1)
    THROW("Failed to set pipe size to " << size << ": " << SysError());

#else
  THROW("Pipe set size is not supported on this platform");
#endif
}


bool PipeEnd::moveFD(handle_t target) {
#ifdef _WIN32
  THROW(CBANG_FUNC << " not supported on Windows");

#else
  if (!isOpen()) THROW("Pipe end not open");

  if (dup2(handle, target) == target) {
    handle = target;
    return true;
  }

  return false;
#endif
}


SmartPointer<std::iostream> PipeEnd::toStream() {
  if (!isOpen()) THROW("Pipe end not open");

  typedef io::stream<io::file_descriptor> stream_t;
  SmartPointer<std::iostream> stream = new stream_t(handle, BOOST_CLOSE_HANDLE);
  handle = INVALID; // Handle will now be closed by the stream

  return stream;
}


void Pipe::create() {
  if (ends[0].isOpen() || ends[1].isOpen()) THROW("Pipe already created");

  PipeEnd::handle_t handles[2];

#ifdef _WIN32
  // Setup security attributes for pipe inheritance
  SECURITY_ATTRIBUTES sAttrs;
  memset(&sAttrs, 0, sizeof(SECURITY_ATTRIBUTES));
  sAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
  sAttrs.bInheritHandle = TRUE;
  sAttrs.lpSecurityDescriptor = 0;

  bool ok = CreatePipe(&handles[0], &handles[1], &sAttrs, 0);
#else
  bool ok = !pipe(handles);
#endif

  if (!ok) THROW("Failed to create pipe: " << SysError());

  getReadEnd ().setHandle(handles[0]);
  getWriteEnd().setHandle(handles[1]);
}


void Pipe::close() {
  ends[0].close();
  ends[1].close();
}
