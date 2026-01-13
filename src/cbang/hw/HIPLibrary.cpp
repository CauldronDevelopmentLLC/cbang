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

#include "HIPLibrary.h"
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
static vector<string> hipLib = {"amdhip64_7.dll", "amdhip64_6.dll",
  "amdhip64.dll"};
#define STDCALL __stdcall

#elif __APPLE__
static const char *hipLib = "/Library/Frameworks/HIP.framework/HIP";
#define STDCALL

#else
static vector<string> hipLib = {"/opt/rocm/lib/libamdhip64.so.7",
  "libamdhip64.so.7", "/opt/rocm/lib/libamdhip64.so.6", "libamdhip64.so.6",
  "/opt/rocm/lib/libamdhip64.so.5", "libamdhip64.so.5",
  "/opt/rocm/lib/libamdhip64.so", "libamdhip64.so"};
#define STDCALL
#endif

#ifdef _WIN32
#define HIP_API __stdcall
#else
#define HIP_API
#endif

#define DYNAMIC_CALL(name, args) {                                  \
    name##_t name = (name##_t)getSymbol(#name);                     \
    if ((err = name args)) THROW(#name "() returned " << err);     \
  }


namespace {
  typedef struct {uint8_t bytes[16];} HIPuuid;
  typedef int (HIP_API *hipInit_t)(unsigned);
  typedef int (HIP_API *hipDriverGetVersion_t)(int *);
  typedef int (HIP_API *hipDeviceGet_t)(int *, int);
  typedef int (HIP_API *hipGetDeviceCount_t)(int *);
  typedef int (HIP_API *hipDeviceComputeCapability_t)(int *, int *, int);
  typedef int (HIP_API *hipDeviceGetAttribute_t)(int *, int, int);
  typedef int (HIP_API *hipDeviceGetName_t)(char *, int, int);
  typedef int (HIP_API *hipDeviceGetUuid_t)(HIPuuid *, int);

  const int HIP_DEVICE_ATTRIBUTE_PCI_BUS_ID = 67;
  const int HIP_DEVICE_ATTRIBUTE_PCI_DEVICE_ID = 68;
  const int HIP_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR = 23;
  const int HIP_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR = 61;
}


HIPLibrary::HIPLibrary(Inaccessible) : DynamicLibrary(hipLib) {
  int err;
  int version;

  DYNAMIC_CALL(hipInit, (0));
  DYNAMIC_CALL(hipDriverGetVersion, (&version));

  VersionU16 driverVersion(version / 1000, (version % 1000) / 10);

  int count = 0;
  DYNAMIC_CALL(hipGetDeviceCount, (&count));

  for (int i = 0; i < count; i++) {
    ComputeDevice cd;

    cd.platform = "HIP";

    // Set indices
    cd.platformIndex = 0; // Only one platform for HIP
    cd.deviceIndex = i;
    cd.gpu = true; // All HIP devices are GPUs

    try {
      int device = 0;

      DYNAMIC_CALL(hipDeviceGet, (&device, i));

      cd.driverVersion = driverVersion;
      cd.computeVersion = VersionU16
        (getAttribute(HIP_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device),
         getAttribute(HIP_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, device));
#if defined(__HIP_PLATFORM_NVIDIA__)
      cd.vendorID = GPUVendor::VENDOR_NVIDIA;
      cd.pciFunction = 0; // Nvidia GPUs are always function 0
#else
      cd.vendorID = GPUVendor::VENDOR_AMD;
      cd.pciFunction = 0;
#endif
      cd.pciBus  = getAttribute(HIP_DEVICE_ATTRIBUTE_PCI_BUS_ID, device);
      cd.pciSlot = getAttribute(HIP_DEVICE_ATTRIBUTE_PCI_DEVICE_ID, device);

      HIPuuid uuid = {{0,}};
      DYNAMIC_CALL(hipDeviceGetUuid, (&uuid, device));
      cd.uuid = UUID(uuid.bytes);

      const unsigned len = 1024;
      char name[len];
      DYNAMIC_CALL(hipDeviceGetName, (name, len, device));
      cd.name = string(name, strnlen(name, len));

      devices.push_back(cd);
    } CATCH_ERROR;
  }
}


const ComputeDevice &HIPLibrary::getDevice(unsigned i) const {
  if (getDeviceCount() <= i) THROW("Invalid HIP device index " << i);
  return devices.at(i);
}


int HIPLibrary::getAttribute(int id, int dev) {
  int err;
  int value = -1;
  DYNAMIC_CALL(hipDeviceGetAttribute, (&value, id, dev));
  return value;
}
