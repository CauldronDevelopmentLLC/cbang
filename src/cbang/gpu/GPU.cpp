/******************************************************************************\

  This file is part of the C! library.  A.K.A the cbang library.

  Copyright (c) 2003-2018, Cauldron Development LLC
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

#include "GPU.h"

#include "GPUType.h"
#include "GPUVendor.h"

#include <cbang/String.h>
#include <cbang/SStream.h>
#include <cbang/Catch.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/log/Logger.h>
#include <cbang/json/JSON.h>

#include <string.h>

using namespace std;
using namespace cb;


string GPU::getIndexKey() const {
  return getVendorIDStr() + ":" + getDeviceIDStr();
}


string GPU::toString() const {
  string s;

  if (getType()) s = SSTR(getType() << ":" << species << " ");
  else s = "UNSUPPORTED: ";

  return s + PCIDevice::toString();
}


string GPU::toRowString() const {
  return
    SSTR(getVendorIDStr() << ':' << getDeviceIDStr() << ':'
         << (uint16_t)getType() << ':' << getSpecies() << ':' << getName());
}


string GPU::toJSONString() const {return JSON::Serializable::toString();}
void GPU::parseJSON(const string &s) {JSON::Serializable::parse(s);}


void GPU::read(const JSON::Value &value) {
  setName(value.getString("name", ""));
  setVendorID((uint16_t)value.getU32("vendor_id", 0));
  setDeviceID((uint16_t)value.getU32("device_id", 0));
  setType(GPUType::parse(value.getString("type", "NONE")));
  setSpecies(value.getU32("species", 0));
}


void GPU::write(JSON::Sink &sink) const {
  sink.beginDict();
  if (!getName().empty()) sink.insert("name", getName());
  sink.insert("vendor_id", getVendorID());
  sink.insert("device_id", getDeviceID());
  if (getType()) sink.insert("type", getType().toString());
  if (getSpecies()) sink.insert("species", getSpecies());
  sink.endDict();
}
