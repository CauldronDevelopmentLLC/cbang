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

#include "GPU.h"
#include "ComputeDevice.h"

#include <string>


namespace cb {
  /// A GPU found on this system, on the PCI bus or not.  The vendor and
  /// device IDs are the hardware's.  The type and species are from the GPU
  /// index.
  class GPUDevice : public GPU {
    std::string id;
    ComputeDevice cuda;
    ComputeDevice hip;
    ComputeDevice opencl;

  public:
    GPUDevice(const std::string &id = std::string()) : id(id) {}

    /// @return The PCI address or "soc:<n>" for GPUs not on the PCI bus.
    const std::string &getID() const {return id;}

    const ComputeDevice &getCUDA() const {return cuda;}
    void setCUDA(const ComputeDevice &cuda) {this->cuda = cuda;}
    const ComputeDevice &getHIP() const {return hip;}
    void setHIP(const ComputeDevice &hip) {this->hip = hip;}
    const ComputeDevice &getOpenCL() const {return opencl;}
    void setOpenCL(const ComputeDevice &opencl) {this->opencl = opencl;}

    std::string getUUID() const;
    bool hasComputeDevice() const;

    /// A GPU is supported if the GPU index lists a non-zero species and at
    /// least one ComputeDevice is found for OpenCL/CUDA/HIP.
    bool isSupported() const {return getSpecies() && hasComputeDevice();}
  };
}
