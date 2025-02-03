/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Exception.h"
#include "String.h"

#include <cbang/config.h>

#include <iomanip>

#include <cbang/debug/Debugger.h>
#include <cbang/json/Sink.h>

using namespace std;
using namespace cb;


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

    for (auto &frame: *trace) {
      if (i++ < 3 && (frame.getFunction().find("Debugger") ||
                      frame.getFunction().find("Exception")))
        continue;
      else stream << "\n  #" << ++count << ' ' << frame;
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
