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

#ifndef CB_CHAKRA_PATH_RESOLVER_H
#define CB_CHAKRA_PATH_RESOLVER_H

#include <string>
#include <vector>


namespace cb {
  namespace js {
    class PathResolver {
      std::vector<std::string> pathStack;
      std::vector<std::string> searchExts;
      std::vector<std::string> searchPaths;

    public:
      PathResolver();
      virtual ~PathResolver() {}

      virtual void pushPath(const std::string &path);
      virtual void popPath();
      const std::string &getCurrentPath() const;

      void addSearchExtensions(const std::string &exts);
      void appendSearchExtension(const std::string &ext);
      void clearSearchExtensions() {searchExts.clear();}
      std::vector<std::string> &getSearchExts() {return searchExts;}
      std::string searchExtensions(const std::string &path) const;

      void addSearchPaths(const std::string &paths);
      std::vector<std::string> &getSearchPaths() {return searchPaths;}
      void clearSearchPaths() {searchPaths.clear();}
      std::string searchPath(const std::string &path) const;
      std::string relativePath(const std::string &path) const;
    };
  }
}

#endif // CB_CHAKRA_PATH_RESOLVER_H
