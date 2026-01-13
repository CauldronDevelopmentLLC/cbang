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

#include "GPU.h"

#include "GPUType.h"
#include "GPUVendor.h"

#include <cbang/String.h>
#include <cbang/SStream.h>
#include <cbang/Catch.h>
#include <cbang/json/JSON.h>

using namespace std;
using namespace cb;


bool GPU::operator<(const GPU &gpu) const {
  if (vendorID != gpu.vendorID) return vendorID < gpu.vendorID;
  return deviceID < gpu.deviceID;
}


void GPU::read(const JSON::Value &value) {
  vendorID = value.getU16("vendor");
  deviceID = value.getU16("device");
  type = (GPUType::enum_t)value.getU16("type", 0);
  species = value.getU16("species", 0);
  description = value.getString("description", "");
}


void GPU::write(JSON::Sink &sink) const {
  sink.beginDict();
  sink.insert("vendor", getVendorID());
  sink.insert("device", getDeviceID());
  sink.insert("type", type);
  sink.insert("species", species);
  sink.insert("description", description);
  sink.endDict();
}
