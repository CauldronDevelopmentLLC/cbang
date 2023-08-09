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

#include "FileFactory.h"

#include <cbang/SmartPointer.h>
#include <cbang/boost/IOStreams.h>


namespace cb {
  class FileDevice : public FileInterface {
    static SmartPointer<FileFactoryBase> factory;

    SmartPointer<FileInterface> impl;

  public:
    typedef char char_type;
    struct category :
      public io::seekable_device_tag, public io::closable_tag {};

    FileDevice(const std::string &path, std::ios::openmode mode,
               int perm = 0644);

    inline static void setFactory(const SmartPointer<FileFactoryBase> &factory)
    {FileDevice::factory = factory;}

    // From FileInterface
    void open(
      const std::string &path, std::ios::openmode mode, int perm) override
    {return impl->open(path, mode, perm);}
    std::streamsize read(char *s, std::streamsize n) override
    {return impl->read(s, n);}
    std::streamsize write(const char *s, std::streamsize n) override
    {return impl->write(s, n);}
    std::streampos seek(std::streampos off, std::ios::seekdir way) override
    {return impl->seek(off, way);}
    bool is_open() const override {return impl->is_open();}
    void close() override {impl->close();}
    std::streamsize size() const override {return impl->size();}
  };


  typedef io::stream<FileDevice> File;
}
