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

#include <cbang/log/Logger.h>


#ifdef DEBUG
#define CBANG_CATCH_LOCATION << "\nCaught at: " << CBANG_FILE_LOCATION
#else // DEBUG
#define CBANG_CATCH_LOCATION
#endif // DEBUG


// NOTE, the variable ``msg`` is made available in all cases during CATCH
// evaluation.  ``MSG`` may contain expressions that use this variable.

#define CBANG_CATCH_CBANG(LEVEL, MSG)                                   \
  catch (const cb::Exception &e) {                                      \
    std::string msg      = e.toString();                                \
    const char *filename = e.getLocation().getFilename().c_str();       \
    CBANG_LOG_LEVEL_LOCATION(                                           \
      LEVEL, "Exception" << MSG << ": " << msg CBANG_CATCH_LOCATION,    \
      filename, e.getLocation().getLine());                             \
  }

#define CBANG_CATCH_STD(LEVEL, MSG)                                     \
  catch (const std::exception &e) {                                     \
    const char *msg = e.what();                                         \
    CBANG_LOG_LEVEL(LEVEL, "std::exception" << MSG << ": " << msg       \
                    CBANG_CATCH_LOCATION);                              \
  }

#define CBANG_CATCH_UNKNOWN(LEVEL, MSG)                                 \
  catch (...) {                                                         \
    const char *msg = "unknown exception";                              \
    CBANG_LOG_LEVEL(LEVEL, msg << MSG CBANG_CATCH_LOCATION);            \
  }


#define CBANG_CATCH_ALL(LEVEL, MSG)                                     \
  CBANG_CATCH_CBANG(LEVEL, MSG)                                         \
  CBANG_CATCH_STD(LEVEL, MSG)                                           \
  CBANG_CATCH_UNKNOWN(LEVEL, MSG)


#ifdef DEBUG
#define CBANG_CATCH(LEVEL, MSG) CBANG_CATCH_CBANG(LEVEL, MSG)
#else // DEBUG
#define CBANG_CATCH(LEVEL, MSG) CBANG_CATCH_ALL(LEVEL, MSG)
#endif // DEBUG

#define CBANG_TRY_CATCH(LEVEL, EXPR, MSG) \
  do {try {EXPR;} CBANG_CATCH(LEVEL, MSG)} while (0)

#define CBANG_CATCH_ERROR CBANG_CATCH(CBANG_LOG_ERROR_LEVEL, "")
#define CBANG_CATCH_WARNING CBANG_CATCH(CBANG_LOG_WARNING_LEVEL, "")
#define CBANG_CATCH_INFO(LEVEL) CBANG_CATCH(CBANG_LOG_INFO_LEVEL(LEVEL), "")

#ifdef DEBUG
#define CBANG_CATCH_DEBUG(LEVEL) CBANG_CATCH(CBANG_LOG_DEBUG_LEVEL(LEVEL), "")
#else
#define CBANG_CATCH_DEBUG(LEVEL)                \
  catch (const cb::Exception &e) {}             \
  catch (const std::exception &e) {}            \
  catch (...) {}
#endif

#define CBANG_TRY_CATCH_ERROR(EXPR)                       \
  CBANG_TRY_CATCH(CBANG_LOG_ERROR_LEVEL, EXPR, "")
#define CBANG_TRY_CATCH_WARNING(EXPR)                     \
  CBANG_TRY_CATCH(CBANG_LOG_WARNING_LEVEL, EXPR, "")
#define CBANG_TRY_CATCH_INFO(LEVEL, EXPR)                 \
  CBANG_TRY_CATCH(CBANG_LOG_INFO_LEVEL(LEVEL), EXPR, "")

#ifdef DEBUG
#define CBANG_TRY_CATCH_DEBUG(LEVEL, EXPR)                \
  CBANG_TRY_CATCH(CBANG_LOG_DEBUG_LEVEL(LEVEL), EXPR, "")
#else
#define CBANG_TRY_CATCH_DEBUG(LEVEL, EXPR)                \
  do {try {EXPR;} CBANG_CATCH_DEBUG(LEVEL)} while (0)
#endif

#ifdef USING_CBANG
#define CATCH(LEVEL, MSG) CBANG_CATCH(LEVEL, MSG)
#define CATCH_ERROR CBANG_CATCH_ERROR
#define CATCH_WARNING CBANG_CATCH_WARNING
#define CATCH_INFO(LEVEL) CBANG_CATCH_INFO(LEVEL)
#define CATCH_DEBUG(LEVEL) CBANG_CATCH_DEBUG(LEVEL)
#define TRY_CATCH(LEVEL, EXPR, MSG) CBANG_TRY_CATCH(LEVEL, EXPR, MSG)
#define TRY_CATCH_ERROR(EXPR) CBANG_TRY_CATCH_ERROR(EXPR)
#define TRY_CATCH_WARNING(EXPR) CBANG_TRY_CATCH_WARNING(EXPR)
#define TRY_CATCH_INFO(LEVEL, EXPR) CBANG_TRY_CATCH_INFO(LEVEL, EXPR)
#define TRY_CATCH_DEBUG(LEVEL, EXPR) CBANG_TRY_CATCH_DEBUG(LEVEL, EXPR)
#endif // USING_CBANG
