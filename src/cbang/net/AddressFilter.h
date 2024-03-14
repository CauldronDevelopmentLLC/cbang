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

#include "AddressRangeSet.h"

#include <string>


namespace cb {
  class SockAddr;

  /**
   * Used to allow or deny clients by IP address.
   * Used a white (allow) and black (deny) list to filter IPs.
   *
   * @see AddressRangeSet
   */
  class AddressFilter {
    AddressRangeSet whiteList;
    AddressRangeSet blackList;

  public:
    /// Add a IP address specification to the deny list.
    void deny(const std::string &spec);
    /// Add a IP address specification to the allow list.
    void allow(const std::string &spec);

    /// Add a IP address range to the deny list.
    void deny(AddressRange &range);
    /// Add a IP address range to the allow list.
    void allow(AddressRange &range);

    /// @return True if the IP address is allowed by the current rules.
    bool isAllowed(const SockAddr &addr) const;

    /// @return True if the IP address is in the white list
    bool isExplicitlyAllowed(const SockAddr &addr) const;
  };
}
