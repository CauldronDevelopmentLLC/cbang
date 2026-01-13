/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include <cbang/SmartPointer.h>

#include <iostream>
#include <string>

typedef struct X509_req_st X509_REQ;


namespace cb {
  class KeyPair;

  class CSR {
    X509_REQ *csr;

    struct private_t;
    private_t *pri;

  public:
    CSR();
    CSR(const std::string &pem);
    ~CSR();

    X509_REQ *getX509_REQ() const {return csr;}

    KeyPair getPublicKey() const;

    void addNameEntry(const std::string &name, const std::string &value);
    bool hasNameEntry(const std::string &name) const;
    std::string getNameEntry(const std::string &name) const;

    bool hasExtension(const std::string &name) const;
    std::string getExtension(const std::string &name) const;
    void addExtension(const std::string &name, const std::string &value);

    void sign(const KeyPair &key, const std::string &digest = "sha256");
    void verify() const;

    std::string toString() const;
    std::string toDER() const;

    std::istream &read(std::istream &stream);
    std::ostream &write(std::ostream &stream) const;
    std::ostream &print(std::ostream &stream) const;
  };

  inline std::istream &operator>>(std::istream &stream, CSR &csr) {
    return csr.read(stream);
  }

  inline std::ostream &operator<<(std::ostream &stream, const CSR &csr) {
    return csr.write(stream);
  }
}
