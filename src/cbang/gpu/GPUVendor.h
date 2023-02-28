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

#ifndef CBANG_ENUM
#ifndef CBANG_GPUVENDOR_H
#define CBANG_GPUVENDOR_H

#define CBANG_ENUM_NAME GPUVendor
#define CBANG_ENUM_PATH cbang/gpu
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PREFIX 7
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_GPUVENDOR_H
#else // CBANG_ENUM

// These match the PCI vendor ID
CBANG_ENUM_VALUE(VENDOR_UNKNOWN, 0)
CBANG_ENUM_VALUE(VENDOR_AMD,     0x1002)
CBANG_ENUM_ALIAS(VENDOR_ATI,     VENDOR_AMD)
CBANG_ENUM_VALUE(VENDOR_NVIDIA,  0x10de)
CBANG_ENUM_VALUE(VENDOR_INTEL,   0x8086)

#endif // CBANG_ENUM
