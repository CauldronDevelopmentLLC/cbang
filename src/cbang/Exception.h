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
  template <typename T, typename DeallocT, typename CounterT>
  class SmartPointer;

  /**
   * Exception is a general purpose exception class.  It is similar to
   * the java Exception class.  A Exception can carry a message, error code,
   * FileLocation and/or a pointer to an exception which was the
   * original cause.  It also can track a file name, line and column which
   * indicates where the exception occured.
   *
   * There are several preprocessor macros that can be used as a convient way
   * to add the current file, line and column where the exception occured.
   * These are:
   *
   *   THROW(const string &message)
   *   THROWC(const string &message, Exception &cause)
   *   THROWX(const string &message, int code)
   *   THROWCX(const string &message, Exception &cause, int code)
   *
   *   THROWS(<message stream>)
   *   THROWCS(<message stream>, Exception &cause)
   *   THROWXS(<message stream>, int code)
   *   THROWCXS(<message stream>, Exception &cause, int code)
   *
   *   DBG_ASSERT(bool condition, const string &message)
   *   DBG_ASSERTS(bool condition, <message stream>)
   *
   * In the versions ending i 'S' <message stream> can be a chain of items to be
   * piped to an std::ostream in order to format the message.  For example:
   *
   *   THROWS("The error number was: " << errno);
   *
   * These stream chains can be of arbitrary length.
   *
   * The DBG_ASSERT macros evaluate condition and throw an exception if false.
   * These macros are compiled out if DEBUG is not defined.
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

    Exception(int code = 0) : code(code) {init();}

    Exception(const std::string &message, int code = 0) :
      message(message), code(code) {
      init();
    }

    Exception(const std::string &message, const FileLocation &location,
              int code = 0) :
      message(message), code(code), location(location) {
      init();
    }

    Exception(const std::string &message, Exception &cause, int code = 0) :
      message(message), code(code) {
      this->cause = new Exception(cause);
      init();
    }

    Exception(const std::string &message, const FileLocation &location,
              const Exception &cause, int code = 0) :
      message(message), code(code), location(location) {
      this->cause = new Exception(cause);
      init();
    }

    /// Copy constructor
    Exception(const Exception &e) :
      message(e.message), code(e.code), location(e.location), cause(e.cause),
      trace(e.trace) {}

    virtual ~Exception() throw() {}

    // From std::exception
    virtual const char *what() const throw() {return message.c_str();}

    const std::string &getMessage() const {return message;}
    void setMessage(const std::string &message) {this->message = message;}

    int getCode() const {return code;}
    void setCode(int code) {this->code = code;}

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
     * Prints the complete exception recuring down to the cause exception if
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

  protected:
    void init();
  };

  /**
   * An stream output operator for Exception.  This allows you to print the
   * text of an exception to a stream like so:
   *
   * . . .
   * } catch (Exception &e) {
   *   cout << e << endl;
   *   return 0;
   * }
   */
  inline std::ostream &operator<<(std::ostream &stream, const Exception &e) {
    return e.print(stream);
  }
}
