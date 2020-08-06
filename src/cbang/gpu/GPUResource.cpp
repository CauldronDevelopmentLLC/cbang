/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2020, Cauldron Development LLC
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

#include "GPUResource.h"
#include "CUDALibrary.h"
#include "OpenCLLibrary.h"
#include "GPUVendor.h"

#include <cbang/Catch.h>

using namespace cb;


namespace {
  template <typename LIB>
  ComputeDevice match(uint16_t vendorID, int16_t busID, int16_t slotID) {
    auto &lib = LIB::instance();

    if (vendorID == GPUVendor::VENDOR_INTEL && busID == -1 && slotID == -1) {
      // Match by vendor ID.  Useful for matching a single Intel GPU
      // Note, There is currently no way to access the PCI bus and slot IDs for
      // Intel GPUs.
      // TODO This assumes there is at most one Intel GPU, that could change.
      for (auto it = lib.begin(); it != lib.end(); it++)
        if (it->gpu && vendorID == it->vendorID)
          return *it;

    } else
      try {
        for (auto it = lib.begin(); it != lib.end(); it++)
          if (it->gpu && busID == it->pciBus && slotID == it->pciSlot)
            return *it;

      } CATCH_WARNING;

    return ComputeDevice();
  }
}


GPUResource::GPUResource(const GPU &gpu, const PCIDevice &pci) :
  GPU(gpu), pci(pci),
  cuda(match<CUDALibrary>(getVendorID(), getBusID(), getSlotID())),
  opencl(match<OpenCLLibrary>(getVendorID(), getBusID(), getSlotID())) {}
