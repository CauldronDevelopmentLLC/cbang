/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include <cbang/io/InputSource.h>
#include <cbang/SmartPointer.h>

#include <vector>


namespace cb {
  class Regex;

  namespace JSON {
    class Value;
    class Sink;

    class YAMLReader {
      InputSource src;

      class Private;
      cb::SmartPointer<Private> pri;

      static Regex nullRE;
      static Regex boolRE;
      static Regex intRE;
      static Regex floatRE;
      static Regex infRE;
      static Regex nanRE;

    public:
      YAMLReader(const InputSource &src);

      void parse(Sink &sink);

      SmartPointer<Value> parse();
      static SmartPointer<Value> parse(const InputSource &src);
      static SmartPointer<Value> parseFile(const std::string &path);

      typedef std::vector<SmartPointer<Value> > docs_t;
      void parse(docs_t &docs);
      static void parse(const InputSource &src, docs_t &docs);
      static void parseFile(const std::string &path, docs_t &docs);

    private:
      void _parse(Sink &sink);
    };
  }
}
