/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#ifndef CB_JS_SIGNATURE_H
#define CB_JS_SIGNATURE_H

#include <cbang/json/Dict.h>


namespace cb {
  namespace js {
    class Signature : public JSON::Dict {
      std::string name;
      bool variable;

    public:
      Signature() : variable(false) {}
      Signature(const std::string &name, const std::string &args) :
        name(name) {parseArgs(args);}
      Signature(const std::string &sig) {parse(sig);}
      Signature(const char *sig) {parse(sig);}

      const std::string &getName() const {return name;}
      void setVariable(bool x) {variable = x;}
      bool isVariable() const {return variable;}

      std::string toString() const;

      static bool isNameStartChar(char c);
      static bool isNameChar(char c);

      void parse(const std::string &sig);
      void parseArgs(const std::string &sig);

    protected:
      static void invalidChar(char c, const std::string &expected);
      static void invalidEnd(const std::string &expected);
    };


    inline static
    std::ostream &operator<<(std::ostream &stream, const Signature &sig) {
      stream << sig.toString();
      return stream;
    }
  }
}

#endif // CB_JS_SIGNATURE_H
