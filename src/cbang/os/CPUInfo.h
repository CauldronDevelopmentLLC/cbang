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

#pragma once

#include <cbang/SmartPointer.h>
#include <cbang/json/Sink.h>

#include <ostream>
#include <string>
#include <set>
#include <map>
#include <cstdint>


namespace cb {
  class CPUInfo {
  protected:
    std::string vendor = "unknown";
    std::string brand  = "unknown";
    uint32_t signature = 0;
    unsigned family    = 0;
    unsigned model     = 0;
    unsigned stepping  = 0;
    unsigned physical  = 0;
    unsigned threads   = 0;

    std::set<std::string> features;

    typedef std::map<std::string, uint64_t> registers_t;
    registers_t registers;

    CPUInfo() {}

  public:
    virtual ~CPUInfo() {}

    static SmartPointer<CPUInfo> create();

    std::string getVendor() const {return vendor;}
    std::string getBrand() const {return brand;}
    uint32_t getSignature() const {return signature;}
    unsigned getFamily() const {return family;}
    unsigned getModel() const {return model;}
    unsigned getStepping() const {return stepping;}
    unsigned getLogicalCPUCount() const {return threads * physical;}
    unsigned getPhysicalCPUCount() const {return physical;}
    unsigned getThreadsPerCore() const {return threads;}

    const std::set<std::string> &getFeatures() const {return features;}
    bool hasFeature(const std::string &feature) const;
    const registers_t &getRegisters() const {return registers;}

    void print(std::ostream &stream) const;
    void write(JSON::Sink &sink) const;
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const CPUInfo &info) {
    info.print(stream);
    return stream;
  }
}
