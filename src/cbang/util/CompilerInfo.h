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

#pragma once

#include <cbang/SStream.h>

#ifdef __INTEL_COMPILER
#ifdef __VERSION__
#define COMPILER CBANG_SSTR(__VERSION__ << " " << __INTEL_COMPILER)

#elif _MSC_VER
#define COMPILER \
  CBANG_SSTR("Intel(R) C++ MSVC " << _MSC_VER << " mode " << __INTEL_COMPILER)
#else
#define COMPILER CBANG_SSTR("Intel(R) C++ " << __INTEL_COMPILER)
#endif

#elif __GNUC__
#define COMPILER "GNU " __VERSION__

#elif _MSC_VER >= 1928
#define COMPILER "Visual C++"
#elif _MSC_VER >= 1927
#define COMPILER "Visual C++ 2019 16.7"
#elif _MSC_VER >= 1926
#define COMPILER "Visual C++ 2019 16.6"
#elif _MSC_VER >= 1925
#define COMPILER "Visual C++ 2019 16.5"
#elif _MSC_VER >= 1924
#define COMPILER "Visual C++ 2019 16.4"
#elif _MSC_VER >= 1923
#define COMPILER "Visual C++ 2019 16.3"
#elif _MSC_VER >= 1922
#define COMPILER "Visual C++ 2019 16.2"
#elif _MSC_VER >= 1921
#define COMPILER "Visual C++ 2019 16.1"
#elif _MSC_VER >= 1920
#define COMPILER "Visual C++ 2019 16.0"
#elif _MSC_VER >= 1916
#define COMPILER "Visual C++ 2017 15.9"
#elif _MSC_VER >= 1915
#define COMPILER "Visual C++ 2017 15.8"
#elif _MSC_VER >= 1914
#define COMPILER "Visual C++ 2017 15.7"
#elif _MSC_VER >= 1913
#define COMPILER "Visual C++ 2017 15.6"
#elif _MSC_VER >= 1912
#define COMPILER "Visual C++ 2017 15.5"
#elif _MSC_VER >= 1911
#define COMPILER "Visual C++ 2017 15.3"
#elif _MSC_VER >= 1910
#define COMPILER "Visual C++ 2017 15.0"
#elif _MSC_VER >= 1900
#define COMPILER "Visual C++ 2015"
#elif _MSC_VER >= 1800
#define COMPILER "Visual C++ 2013"
#elif _MSC_VER >= 1700
#define COMPILER "Visual C++ 2012"
#elif _MSC_VER >= 1600
#define COMPILER "Visual C++ 2010"
#elif _MSC_VER >= 1500
#define COMPILER "Visual C++ 2008"
#elif _MSC_VER >= 1400
#define COMPILER "Visual C++ 2005"
#elif _MSC_VER >= 1310
#define COMPILER "Visual C++ 2003"
#elif _MSC_VER > 1300
#define COMPILER "Visual C++ 2002"
#else
#define COMPILER "Unknown"
#endif

#if defined(_WIN64) || defined(__x86_64__) || defined(__LP64__) || \
  defined(__aarch64__)
#define COMPILER_BITS 64
#else
#define COMPILER_BITS 32
#endif
