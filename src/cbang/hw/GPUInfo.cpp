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

#include "GPUInfo.h"
#include "GPUIndex.h"
#include "PCIInfo.h"
#include "OpenCLLibrary.h"
#include "CUDALibrary.h"
#include "HIPLibrary.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <map>

using namespace std;
using namespace cb;


namespace {
  typedef map<string, GPUDevice> found_t;


  template <typename LIB>
  void add(found_t &found, void (GPUDevice::*set)(const ComputeDevice &)) {
    try {
      for (auto &cd: LIB::instance()) {
        LOG_DEBUG(3, cd);

        if (!cd.isValid() || !cd.gpu || !cd.isIDValid()) continue;

        auto &gpu = found.try_emplace(cd.getID(), cd.getID()).first->second;
        (gpu.*set)(cd);

        // Hardware IDs of GPUs not on the PCI bus
        if (!gpu.getVendorID() && 0 < cd.vendorID && cd.vendorID <= 0xffff)
          gpu.setVendorID(cd.vendorID);
        if (!gpu.getDeviceID() && 0 < cd.deviceID) gpu.setDeviceID(cd.deviceID);

        // Prefer the driver's name
        if (gpu.getDescription().empty()) gpu.setDescription(cd.name);
      }

    } catch (const Exception &e) {
      LOG_DEBUG(3, LIB::getName() << " not supported: " << e.getMessage());
    }
  }
}


GPUInfo::GPUInfo(const GPUIndex &index) {
  found_t found;

  add<OpenCLLibrary>(found, &GPUDevice::setOpenCL);
#ifndef __APPLE__
  add<CUDALibrary>  (found, &GPUDevice::setCUDA);
#endif
  add<HIPLibrary>   (found, &GPUDevice::setHIP);

  // Hardware IDs of GPUs on the PCI bus
  PCIInfo pci;
  for (auto &dev: pci) {
    auto it = found.find(dev.getID());

    if (it == found.end()) {
      // Also list GPUs without a ComputeDevice, e.g. missing drivers
      if (!index.find(dev.getVendorID(), dev.getDeviceID()).getType()) continue;
      it = found.try_emplace(dev.getID(), dev.getID()).first;
    }

    it->second.setVendorID(dev.getVendorID());
    it->second.setDeviceID(dev.getDeviceID());
  }

  // Look up type and species
  for (auto &p: found) {
    auto &gpu = p.second;
    GPU entry = index.find(gpu.getVendorID(), gpu.getDeviceID());

    gpu.setType(entry.getType());
    gpu.setSpecies(entry.getSpecies());
    if (gpu.getDescription().empty())
      gpu.setDescription(entry.getDescription());

    gpus.push_back(gpu);
  }
}
