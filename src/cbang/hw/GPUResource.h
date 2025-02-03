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

#include "GPU.h"
#include "ComputeDevice.h"
#include "PCIInfo.h"


namespace cb {
  class GPUResource : public GPU {
    PCIDevice pci;
    ComputeDevice cuda;
    ComputeDevice hip;
    ComputeDevice opencl;

  public:
    GPUResource() {}
    GPUResource(const GPU &gpu, const PCIDevice &pci);

    const PCIDevice &getPCI() const {return pci;}
    const ComputeDevice &getCUDA() const {return cuda;}
    void setCUDA(const ComputeDevice &cuda) {this->cuda = cuda;}
    const ComputeDevice &getHIP() const {return hip;}
    void setHIP(const ComputeDevice &hip) {this->hip = hip;}
    const ComputeDevice &getOpenCL() const {return opencl;}
    void setOpenCL(const ComputeDevice &opencl) {this->opencl = opencl;}

    int16_t getBusID() const {return pci.getBusID();}
    int16_t getSlotID() const {return pci.getSlotID();}
    int16_t getFunctionID() const {return pci.getFunctionID();}
  };
}
