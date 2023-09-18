/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "Exception.h"
#include "String.h"

#include <cbang/config.h>

#include <iomanip>

#include <cbang/debug/Debugger.h>
#include <cbang/json/Sink.h>

using namespace std;
using namespace cb;


#if defined(_WIN32) && !defined(__MINGW32__)
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>


namespace {
  const char *win32_code_to_string(int code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:         return "Access violation";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "Array bounds exceeded";
    case EXCEPTION_BREAKPOINT:               return "Breakpoint";
    case EXCEPTION_DATATYPE_MISALIGNMENT:    return "Datatype misalignment";
    case EXCEPTION_FLT_DENORMAL_OPERAND:     return "Float denormal operand";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "Float divide by zero";
    case EXCEPTION_FLT_INEXACT_RESULT:       return "Float inexact result";
    case EXCEPTION_FLT_INVALID_OPERATION:    return "Float invalid operation";
    case EXCEPTION_FLT_OVERFLOW:             return "Float overflow";
    case EXCEPTION_FLT_STACK_CHECK:          return "Float stack check";
    case EXCEPTION_FLT_UNDERFLOW:            return "Float underflow";
    case EXCEPTION_ILLEGAL_INSTRUCTION:      return "Illegal instruction";
    case EXCEPTION_IN_PAGE_ERROR:            return "In page error";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "Tnteger divide by zero";
    case EXCEPTION_INT_OVERFLOW:             return "Integer overflow";
    case EXCEPTION_INVALID_DISPOSITION:      return "Invalid disposition";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception";
    case EXCEPTION_PRIV_INSTRUCTION:         return "Private instruction";
    case EXCEPTION_SINGLE_STEP:              return "Single step";
    case EXCEPTION_STACK_OVERFLOW:           return "Stack overflow";
    default:                                 return "Unknown";
    }
  }


  extern "C" void convert_win32_exception(unsigned x, EXCEPTION_POINTERS *e){
    const char *msg = win32_code_to_string(e->ExceptionRecord->ExceptionCode);
    THROW("Win32: 0x" << hex << x << ": " << msg);
  }
}
#endif // _WIN32


#ifdef DEBUG
bool Exception::enableStackTraces = true;
#else
bool Exception::enableStackTraces = false;
#endif

bool Exception::printLocations = true;
unsigned Exception::causePrintLevel = 10;


Exception::Exception(const string &message, int code,
                     const FileLocation &location,
                     const SmartPointer<Exception> &cause) :
  message(message), code(code), location(location), cause(cause) {

#if defined(_WIN32) && !defined(__MINGW32__)
  _set_se_translator(convert_win32_exception);
#endif

#ifdef HAVE_CBANG_BACKTRACE
  if (enableStackTraces) {
    trace = new StackTrace();
    Debugger::instance().getStackTrace(*trace, false);
  }
#endif
}


ostream &Exception::print(ostream &stream, unsigned level) const {
  if (code) stream << code << ": ";

  stream << message;

  if (printLocations && !location.isEmpty())
    stream << "\n       At: " << location;

  if (!trace.isNull()) {
    Debugger::instance().resolve(*trace);

    unsigned count = 0;
    unsigned i = 0;

    StackTrace::iterator it;
    for (it = trace->begin(); it != trace->end(); it++, i++) {
      if (i < 3 && (it->getFunction().find("Debugger") ||
                    it->getFunction().find("Exception")))
        continue;
      else stream << "\n  #" << ++count << ' ' << *it;
    }
  }

  if (!cause.isNull()) {
    stream << endl;

    if (level > causePrintLevel) {
      stream << "Aborting exception dump due to cause print level limit! "
        "Increase Exception::causePrintLevel to see more.";

    } else {
      stream << "Caused by: ";
      cause->print(stream, level + 1);
    }
  }

  return stream;
}


string Exception::toString() const {return SSTR(*this);}


void Exception::write(JSON::Sink &sink, bool withDebugInfo) const {
  sink.beginDict();

  if (!message.empty()) sink.insert("message", message);
  if (code) sink.insert("code", code);

  if (withDebugInfo) {
    if (!location.isEmpty()) {
      sink.beginInsert("location");
      location.write(sink);
    }

    if (trace.isSet()) {
      sink.beginInsert("trace");
      trace->write(sink);
    }
  }

  if (cause.isSet()) {
    sink.beginInsert("cause");
    cause->write(sink, withDebugInfo);
  }

  sink.endDict();
}
