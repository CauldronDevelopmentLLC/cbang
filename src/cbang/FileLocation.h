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

#include <string>
#include <iostream>
#include <cstdint>


namespace cb {
  namespace JSON {class Sink;}

  /**
   * This class is mainly used by Exception, but can be used
   * as a general class for recording a line and column location
   * with in a file.
   */
  class FileLocation {
  protected:
    std::string filename;
    std::string function;
    int32_t line;
    int32_t col;

  public:
    /**
     * Construct a default FileLocation with an empty value.
     */
    FileLocation() : line(-1), col(-1) {}

    /**
     * Copy constructor.
     */
    FileLocation(const FileLocation &x) :
      filename(x.filename), function(x.function), line(x.line), col(x.col) {}

    /**
     * @param filename The name of the file.
     * @param line The line with that file.
     * @param col The column on that line.
     */
    FileLocation(const std::string &filename, const int32_t line = -1,
                 const int32_t col = -1) :
      filename(filename), line(line), col(col) {}

    FileLocation(const std::string &filename, const std::string &function,
                 const int32_t line = -1,  const int32_t col = -1) :
      filename(filename), function(function), line(line), col(col) {}

    virtual ~FileLocation() {}

    const std::string &getFilename() const {return filename;}
    void setFilename(const std::string &filename) {this->filename = filename;}

    const std::string &getFunction() const {return function;}
    void setFunction(const std::string &function) {this->function = function;}

    std::string getFileLineColumn() const;

    /// @return -1 if no line was set the line number otherwise
    int32_t getLine() const {return line;}
    void setLine(int32_t line) {this->line = line;}
    void incLine() {line++;}

    /// @return -1 of no column was set the column number otherwise
    int32_t getCol() const {return col;}
    void setCol(int32_t col) {this->col = col;}
    void incCol() {col++;}

    /// @return True of nothing set
    bool isEmpty() const;

    bool operator==(const FileLocation &o) const;
    bool operator!=(const FileLocation &o) const {return *this == o;}

    void print(std::ostream &stream) const;
    void write(cb::JSON::Sink &sink) const;
  };

  /**
   * Print a file location to a stream.  The format is as follows.
   *
   * filename[:line[:col]]
   *
   * If no line or column has been set then they will not be displayed.
   *
   * @return A reference to the passed stream.
   */
  static inline
  std::ostream &operator<<(std::ostream &stream, const FileLocation &fl) {
    fl.print(stream);
    return stream;
  }


  static inline std::string functionName(const std::string &fsig) {
    size_t p = fsig.find('(');
    size_t s = fsig.find_last_of(' ', p);
    size_t start = s == std::string::npos ? 0 : (s + 1);
    size_t end = p == std::string::npos ? fsig.length() : p;

    return fsig.substr(start, end - start);
  }
}

#ifdef _WIN32
#define CBANG_FUNC __func__
#else
#define CBANG_FUNC cb::functionName(__PRETTY_FUNCTION__)
#endif

#ifdef DEBUG
#define CBANG_FILE_LOCATION cb::FileLocation(__FILE__, CBANG_FUNC, __LINE__, -1)

#else // DEBUG
#define CBANG_FILE_LOCATION cb::FileLocation()
#endif // DEBUG

#ifdef USING_CBANG
#define FILE_LOCATION CBANG_FILE_LOCATION
#endif
