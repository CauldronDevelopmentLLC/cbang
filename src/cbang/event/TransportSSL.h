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

#include <cbang/config.h>
#ifdef HAVE_OPENSSL

#include "Transport.h"

#include <cbang/openssl/SSL.h>


namespace cb {
  namespace Event {
    class TransportSSL : public Transport {
      SmartPointer<SSL> ssl;

    public:
      TransportSSL(const SmartPointer<SSL> &ssl);

      // From Transport
      bool wantsRead() const {return ssl->wantsRead();}
      bool wantsWrite() const {return ssl->wantsWrite();}
      int read(Buffer &buffer, unsigned length);
      int write(Buffer &buffer, unsigned length);

      bool checkError(int ret);
    };
  }
}

#endif // HAVE_OPENSSL
