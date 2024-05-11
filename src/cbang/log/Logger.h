/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "LogListener.h"

#include <cbang/SStream.h>
#include <cbang/SmartPointer.h>
#include <cbang/Exception.h>

#include <cbang/util/Singleton.h>
#include <cbang/comp/Compression.h>
#include <cbang/thread/Lockable.h>

#include <ostream>
#include <string>
#include <map>
#include <set>
#include <vector>


namespace cb {
  class Option;
  class Options;
  class CommandLine;
  class RateSet;
  class Mutex;
  template <typename T> class ThreadLocalStorage;

  namespace JSON {class Sink;}

  /**
   * Handles all information logging.  Both to the screen and optionally to
   * a log file.
   * Logging should be done through the provided macros e.g. LOG_ERROR
   * The logging macros allow C++ iostreaming.  For example.
   *   LOG_INFO(3, "The value was " << x << ".");
   */
  class Logger : public Lockable, public Singleton<Logger> {
  public:
    enum log_level_t {
      LEVEL_RAW     = 0,
      LEVEL_ERROR   = 1 << 0,
      LEVEL_WARNING = 1 << 1,
      LEVEL_INFO    = 1 << 2,
      LEVEL_DEBUG   = 1 << 3,
      LEVEL_MASK    = (1 << 4) - 1
    };

  private:
    static Mutex mutex;
    std::string logFilename;

#ifdef CBANG_DEBUG_LEVEL
    unsigned    verbosity           = CBANG_DEBUG_LEVEL;
#else
    unsigned    verbosity           = 1;
#endif
    bool        logCRLF             = false;
#ifdef DEBUG
    bool        logDebug            = true;
#else
    bool        logDebug            = false;
#endif
    bool        logTime             = true;
    bool        logDate             = false;
    uint64_t    logDatePeriodically = 0;
    bool        logShortLevel       = false;
    bool        logLevel            = true;
    bool        logPrefix           = false;
    bool        logDomain           = false;
    bool        logSimpleDomains    = true;
    bool        logThreadID         = false;
    bool        logHeader           = true;
    bool        logNoInfoHeader     = false;
    bool        logColor            = true;
    bool        logToScreen         = true;
    bool        logTrunc            = false;
    bool        logRotate           = true;
    Compression logRotateCompression;
    unsigned    logRotateMax        = 0;
    std::string logRotateDir        = "logs";
    uint32_t    logRotatePeriod     = 0;
    unsigned    logRates            = 0;

    SmartPointer<RateSet> rates;
    std::map<std::string, std::string> rateMessages;

    SmartPointer<ThreadLocalStorage<unsigned>> threadIDStorage;
    SmartPointer<ThreadLocalStorage<std::string>>   prefixStorage;

    SmartPointer<std::ostream> logFile;
    SmartPointer<std::ostream> screenStream;
    std::vector<SmartPointer<LogListener>> listeners;

    mutable unsigned idWidth = 1;

    typedef std::map<std::string, int> domain_levels_t;
    domain_levels_t infoDomainLevels;
    domain_levels_t debugDomainLevels;

    typedef std::set<std::string> domain_traces_t;
    domain_traces_t domainTraces;

    uint64_t lastDate;
    uint64_t lastRotate;

  public:
    Logger(Inaccessible);
    ~Logger();

    // From Lockable
    bool lock(double timeout = -1) const override;
    void unlock() const override;
    bool tryLock() const override;

    void addOptions(Options &options);
    void setOptions(Options &options);
    int domainLevelsAction(Option &option);

    void startLogFile(const std::string &filename);
    bool getLogFileStarted() const {return !logFile.isNull();}

    void setScreenStream(std::ostream &stream);
    void setScreenStream(const SmartPointer<std::ostream> &stream);

    void addListener(const SmartPointer<LogListener> &l)
      {listeners.push_back(l);}

    void setVerbosity(unsigned x)       {verbosity        = x;}
    void setLogDebug(bool x)            {logDebug         = x;}
    void setLogCRLF(bool x)             {logCRLF          = x;}
    void setLogTime(bool x)             {logTime          = x;}
    void setLogDate(bool x)             {logDate          = x;}
    void setLogShortLevel(bool x)       {logShortLevel    = x;}
    void setLogLevel(bool x)            {logLevel         = x;}
    void setLogPrefix(bool x)           {logPrefix        = x;}
    void setLogDomain(bool x)           {logDomain        = x;}
    void setLogSimpleDomains(bool x)    {logSimpleDomains = x;}
    void setLogThreadID(bool x)         {logThreadID      = x;}
    void setLogNoInfoHeader(bool x)     {logNoInfoHeader  = x;}
    void setLogHeader(bool x)           {logHeader        = x;}
    void setLogColor(bool x)            {logColor         = x;}
    void setLogToScreen(bool x)         {logToScreen      = x;}
    void setLogTruncate(bool x)         {logTrunc         = x;}
    void setLogRotate(bool x)           {logRotate        = x;}
    void setLogRotateMax(unsigned x)    {logRotateMax     = x;}
    void setLogRotatePeriod(uint32_t x) {logRotatePeriod  = x;}
    void setLogRates(unsigned x)        {logRates         = x;}
    void setLogDomainLevels(const std::string &levels);

    unsigned getVerbosity() const {return verbosity;}
    bool getLogCRLF() const {return logCRLF;}
    unsigned getHeaderWidth() const;
    const SmartPointer<RateSet> &getRates() const {return rates;}

    void setThreadID(unsigned id);
    unsigned getThreadID() const;
    void setPrefix(const std::string &prefix);
    std::string getPrefix() const;

    std::string simplifyDomain(const std::string &domain) const;
    int domainVerbosity(const std::string &domain, int level) const;
    static char getLevelChar(int level);
    std::string getHeader(const std::string &domain, int level) const;
    const char *startColor(int level) const;
    const char *endColor(int level) const;

    void writeRates(JSON::Sink &sink) const;

    // These functions should not be called directly.  Use the macros.
    bool enabled(const std::string &domain, int level) const;
    typedef SmartPointer<std::ostream> LogStream;
    LogStream createStream(const std::string &domain, int level,
                           const std::string &prefix = std::string(),
                           const char *filename = 0, int line = 0);

  protected:
    void rateMessage(const std::string &key, const std::string &msg);
    void logBar(const std::string &msg, uint64_t ts) const;
    void write(const char *s, std::streamsize n);
    void write(const std::string &s);
    bool flush();

    friend class LogDevice;
  };
}

#ifndef CBANG_LOG_DOMAIN
#define CBANG_LOG_DOMAIN __FILE__
#endif

#ifndef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX ""
#endif

// Log levels
#define CBANG_LOG_RAW_LEVEL      cb::Logger::LEVEL_RAW
#define CBANG_LOG_ERROR_LEVEL    cb::Logger::LEVEL_ERROR
#define CBANG_LOG_WARNING_LEVEL  cb::Logger::LEVEL_WARNING
#define CBANG_LOG_DEBUG_LEVEL(x) (cb::Logger::LEVEL_DEBUG + ((x) << 8))
#define CBANG_LOG_INFO_LEVEL(x)  (cb::Logger::LEVEL_INFO  + ((x) << 8))


// Check if logging level is enabled
#define CBANG_LOG_ENABLED(domain, level)                        \
  cb::Logger::instance().enabled(domain, level)
#ifdef DEBUG
#define CBANG_LOG_DEBUG_ENABLED(x)                              \
  CBANG_LOG_ENABLED(CBANG_LOG_DOMAIN, CBANG_LOG_DEBUG_LEVEL(x))
#else
#define CBANG_LOG_DEBUG_ENABLED(x) false
#endif
#define CBANG_LOG_INFO_ENABLED(x)                               \
  CBANG_LOG_ENABLED(CBANG_LOG_DOMAIN, CBANG_LOG_INFO_LEVEL(x))


// Create logger streams
// Warning these macros lock the Logger until they are deallocated
#define CBANG_LOG_STREAM_LOCATION(domain, level, file, line)            \
  cb::Logger::instance()                                                \
    .createStream(domain, level, CBANG_SSTR(CBANG_LOG_PREFIX), file, line)

#define CBANG_LOG_STREAM(domain, level)                               \
  CBANG_LOG_STREAM_LOCATION(domain, level, __FILE__, __LINE__)

#define CBANG_LOG_RAW_STREAM()                                        \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, CBANG_LOG_RAW_LEVEL)
#define CBANG_LOG_ERROR_STREAM()                                      \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, CBANG_LOG_ERROR_LEVEL)
#define CBANG_LOG_WARNING_STREAM()                                    \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, CBANG_LOG_WARNING_LEVEL)
#define CBANG_LOG_DEBUG_STREAM(level)                                 \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, CBANG_LOG_DEBUG_LEVEL(level))
#define CBANG_LOG_INFO_STREAM(level)                                  \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, CBANG_LOG_INFO_LEVEL(level))


// Log messages
// The 'do {...} while (false)' loop lets this compile correctly:
//   if (expr) CBANG_LOG(...); else ...
#define CBANG_LOG_LOCATION(domain, level, msg, file, line)            \
  do {                                                                \
    if (CBANG_LOG_ENABLED(domain, level))                             \
      *CBANG_LOG_STREAM_LOCATION(domain, level, file, line) << msg;   \
  } while (false)

#define CBANG_LOG(domain, level, msg)                           \
  CBANG_LOG_LOCATION(domain, level, msg, __FILE__, __LINE__)

#define CBANG_LOG_LEVEL_LOCATION(level, msg, file, line)        \
  CBANG_LOG_LOCATION(CBANG_LOG_DOMAIN, level, msg, file, line)

#define CBANG_LOG_LEVEL(level, msg) CBANG_LOG(CBANG_LOG_DOMAIN, level, msg)

#define CBANG_LOG_RAW(msg)      CBANG_LOG_LEVEL(CBANG_LOG_RAW_LEVEL,     msg)
#define CBANG_LOG_ERROR(msg)    CBANG_LOG_LEVEL(CBANG_LOG_ERROR_LEVEL,   msg)
#define CBANG_LOG_WARNING(msg)  CBANG_LOG_LEVEL(CBANG_LOG_WARNING_LEVEL, msg)
#define CBANG_LOG_INFO(x, msg)  CBANG_LOG_LEVEL(CBANG_LOG_INFO_LEVEL(x), msg)

#ifdef DEBUG
#define CBANG_LOG_DEBUG(x, msg) CBANG_LOG_LEVEL(CBANG_LOG_DEBUG_LEVEL(x), msg)
#else
#define CBANG_LOG_DEBUG(x, msg) do {} while (false)
#endif


#ifdef USING_CBANG
#define LOG_RAW_LEVEL              CBANG_LOG_RAW_LEVEL
#define LOG_ERROR_LEVEL            CBANG_LOG_ERROR_LEVEL
#define LOG_WARNING_LEVEL          CBANG_LOG_WARNING_LEVEL
#define LOG_DEBUG_LEVEL(x)         CBANG_LOG_DEBUG_LEVEL(x)
#define LOG_INFO_LEVEL(x)          CBANG_LOG_INFO_LEVEL(x)

#define LOG_ENABLED(domain, level) CBANG_LOG_ENABLED(domain, level)
#define LOG_DEBUG_ENABLED(x)       CBANG_LOG_DEBUG_ENABLED(x)
#define LOG_INFO_ENABLED(x)        CBANG_LOG_INFO_ENABLED(x)

#define LOG_STREAM(domain, level)  CBANG_LOG_STREAM(domain, level)
#define LOG_RAW_STREAM()           CBANG_LOG_RAW_STREAM()
#define LOG_ERROR_STREAM()         CBANG_LOG_ERROR_STREAM()
#define LOG_WARNING_STREAM()       CBANG_LOG_WARNING_STREAM()
#define LOG_DEBUG_STREAM(level)    CBANG_LOG_DEBUG_STREAM(level)
#define LOG_INFO_STREAM(level)     CBANG_LOG_INFO_STREAM(level)

#define LOG(domain, level, msg)    CBANG_LOG(domain, level, msg)
#define LOG_LEVEL(level, msg)      CBANG_LOG_LEVEL(level, msg)
#define LOG_RAW(msg)               CBANG_LOG_RAW(msg)
#define LOG_ERROR(msg)             CBANG_LOG_ERROR(msg)
#define LOG_WARNING(msg)           CBANG_LOG_WARNING(msg)
#define LOG_INFO(x, msg)           CBANG_LOG_INFO(x, msg)
#define LOG_DEBUG(x, msg)          CBANG_LOG_DEBUG(x, msg)
#endif // USING_CBANG
