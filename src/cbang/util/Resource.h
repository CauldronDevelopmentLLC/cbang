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

#pragma once

#include <cbang/Exception.h>

#include <string>
#include <ostream>


namespace cb {
  class Resource {
  public:
    const char *name;

    Resource(const char *name) : name(name) {}
    virtual ~Resource() {}

    const char *getName() const {return name;}

    // Directory methods
    virtual bool isDirectory() const {return false;}
    virtual const Resource *find(const std::string &path) const
    {CBANG_THROW(CBANG_FUNC << "() not supported by resource");}
    virtual const Resource *getChild(unsigned i) const
    {CBANG_THROW(CBANG_FUNC << "() not supported by resource");}

    // File methods
    virtual const char *getData() const
    {CBANG_THROW(CBANG_FUNC << "() not supported by resource");}
    virtual unsigned getLength() const
    {CBANG_THROW(CBANG_FUNC << "() not supported by resource");}
    virtual std::string toString() const
    {CBANG_THROW(CBANG_FUNC << "() not supported by resource");}

    const Resource &get(const std::string &path) const;
  };


  class FileResource : public Resource {
  public:
    const char *data;
    const unsigned length;

    FileResource(const char *name, const char *data, unsigned length) :
      Resource(name), data(data), length(length) {}

    // From Resource
    const char *getData() const override {return data;}
    unsigned getLength() const override {return length;}
    std::string toString() const override {return std::string(data, length);}
  };


  class DirectoryResource : public Resource {
  public:
    const Resource **children;

    DirectoryResource(const char *name, const Resource **children) :
      Resource(name), children(children) {}

    // From Resource
    bool isDirectory() const override {return true;}
    const Resource *find(const std::string &path) const override;
    const Resource *getChild(unsigned i) const  override {return children[i];}
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const Resource &r) {
    stream.write(r.getData(), r.getLength());
    return stream;
  }
}
