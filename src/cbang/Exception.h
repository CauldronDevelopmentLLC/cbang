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

#pragma once

#include "FileLocation.h"
#include "SmartPointer.h"
#include "Throw.h"

#include <cbang/debug/StackTrace.h>

#include <vector>
#include <string>
#include <iostream>
#include <exception>


namespace cb {
  // Forward Declarations
  template <typename T, typename DeallocT, typename CounterT, bool weak>
  class SmartPointer;

  /**
   * Exception is a general purpose exception class containing:
   *
   *   - A text message
   *   - A numeric code
   *   - FileLocation indicating where the exception occurred.
   *   - A pointer to an exception which was the original cause.
   *   - A stack trace.
   */
  class Exception : public std::exception {
  private:
    std::string message;
    int code;
    FileLocation location;
    SmartPointer<Exception> cause;
    SmartPointer<StackTrace> trace;

  public:
    static bool enableStackTraces;
    static bool printLocations;
    static unsigned causePrintLevel;

    Exception(const std::string &message = "", int code = 0,
              const FileLocation &location = FileLocation(),
              const SmartPointer<Exception> &cause = 0);

    Exception(const std::string &message, const FileLocation &location,
              int code = 0) : Exception(message, code, location) {}

    Exception(const std::string &message, const Exception &cause,
              int code = 0) :
      Exception(message, code, FileLocation(), new Exception(cause)) {}

    Exception(const std::string &message, const FileLocation &location,
              const Exception &cause, int code = 0) :
      Exception(message, code, location, new Exception(cause)) {}

    /// Copy constructor
    Exception(const Exception &e) :
      message(e.message), code(e.code), location(e.location), cause(e.cause),
      trace(e.trace) {}

    virtual ~Exception() {}

    // From std::exception
    virtual const char *what() const throw() override {return message.c_str();}

    const std::string &getMessage() const {return message;}
    void setMessage(const std::string &message) {this->message = message;}


    std::string getMessages() const {
      if (cause.isNull()) return message;
      return message + ": " + cause->getMessages();
    }


    int getCode() const {return code;}
    void setCode(int code) {this->code = code;}


    int getTopCode() const {
      if (code || cause.isNull()) return code;
      return cause->getTopCode();
    }


    const FileLocation &getLocation() const {return location;}
    void setLocation(const FileLocation &location) {this->location = location;}

    /**
     * @return A SmartPointer to the Exception that caused this
     *         exception or NULL.
     */
    SmartPointer<Exception> getCause() const {return cause;}
    void setCause(SmartPointer<Exception> cause) {this->cause = cause;}
    SmartPointer<StackTrace> getStackTrace() const {return trace;}
    void setStackTrace(SmartPointer<StackTrace> trace) {this->trace = trace;}

    /**
     * Prints the complete exception recurring down to the cause exception if
     * not null.  WARNING: If there are many layers of causes this function
     * could print a very large amount of data.  This can be limited by
     * setting the causePrintLevel variable.
     *
     * @param stream The output stream.
     * @param level The current cause print level.
     *
     * @return A reference to the passed stream.
     */
    std::ostream &print(std::ostream &stream, unsigned level = 0) const;

    std::string toString() const;

    void write(cb::JSON::Sink &sink, bool withDebugInfo = true) const;
  };

  /**
   * An stream output operator for Exception.  This allows you to print the
   * text of an exception to a stream like so:
   *
   * . . .
   * } catch (Exception &e) {
   *   cout << e << endl;
   * }
   */
  inline std::ostream &operator<<(std::ostream &stream, const Exception &e) {
    return e.print(stream);
  }
}
