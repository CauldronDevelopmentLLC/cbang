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

#include "CUDALibrary.h"
#include "GPUVendor.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>
#include <cbang/util/UUID.h>

#include <cstring>

using namespace std;
using namespace cb;


#undef CBANG_EXCEPTION
#define CBANG_EXCEPTION DynamicLibraryException

#ifdef _WIN32
static const char *cudaLib = "nvcuda.dll";
#define STDCALL __stdcall

#elif __APPLE__
static const char *cudaLib = "/Library/Frameworks/CUDA.framework/CUDA";
#define STDCALL

#else
static const char *cudaLib = "libcuda.so";
#define STDCALL
#endif

#ifdef _WIN32
#define CUDA_API __stdcall
#else
#define CUDA_API
#endif

#define DYNAMIC_CALL(name, args) {                                  \
    name##_t name = (name##_t)getSymbol(#name);                     \
    if ((err = name args)) THROW(#name "() returned " << err);     \
  }


namespace {
  typedef struct {uint8_t bytes[16];} CUuuid;
  typedef int (CUDA_API *cuInit_t)(unsigned);
  typedef int (CUDA_API *cuDriverGetVersion_t)(int *);
  typedef int (CUDA_API *cuDeviceGet_t)(int *, int);
  typedef int (CUDA_API *cuDeviceGetCount_t)(int *);
  typedef int (CUDA_API *cuDeviceComputeCapability_t)(int *, int *, int);
  typedef int (CUDA_API *cuDeviceGetAttribute_t)(int *, int, int);
  typedef int (CUDA_API *cuDeviceGetName_t)(char *, int, int);
  typedef int (CUDA_API *cuDeviceGetUuid_t)(CUuuid *, int);

  const int CU_DEVICE_ATTRIBUTE_PCI_BUS_ID = 33;
  const int CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID = 34;
  const int CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR = 75;
  const int CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR = 76;
}


CUDALibrary::CUDALibrary(Inaccessible) : DynamicLibrary(cudaLib) {
  int err;
  int version;

  DYNAMIC_CALL(cuInit, (0));
  DYNAMIC_CALL(cuDriverGetVersion, (&version));

  VersionU16 driverVersion(version / 1000, (version % 1000) / 10);

  int count = 0;
  DYNAMIC_CALL(cuDeviceGetCount, (&count));

  for (int i = 0; i < count; i++) {
    ComputeDevice cd;

    cd.platform = "CUDA";

    // Set indices
    cd.platformIndex = 0; // Only one platform for CUDA
    cd.deviceIndex = i;
    cd.gpu = true; // All CUDA devices are GPUs

    try {
      int device = 0;

      DYNAMIC_CALL(cuDeviceGet, (&device, i));

      cd.driverVersion = driverVersion;
      cd.computeVersion = VersionU16(
        getAttribute(CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device),
        getAttribute(CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, device));
      cd.vendorID = GPUVendor::VENDOR_NVIDIA; // Only vendor for CUDA
      cd.pciBus  = getAttribute(CU_DEVICE_ATTRIBUTE_PCI_BUS_ID, device);
      cd.pciSlot = getAttribute(CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID, device);
      cd.pciFunction = 0; // NVidia GPUs are always function 0

      CUuuid uuid = {{0,}};
      DYNAMIC_CALL(cuDeviceGetUuid, (&uuid, device));
      cd.uuid = UUID(uuid.bytes);

      const unsigned len = 1024;
      char name[len];
      DYNAMIC_CALL(cuDeviceGetName, (name, len, device));
      cd.name = string(name, strnlen(name, len));

      devices.push_back(cd);
    } CATCH_ERROR;
  }
}


const ComputeDevice &CUDALibrary::getDevice(unsigned i) const {
  if (getDeviceCount() <= i) THROW("Invalid CUDA device index " << i);
  return devices.at(i);
}


int CUDALibrary::getAttribute(int id, int dev) {
  int err;
  int value = -1;
  DYNAMIC_CALL(cuDeviceGetAttribute, (&value, id, dev));
  return value;
}
