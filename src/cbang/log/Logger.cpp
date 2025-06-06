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

#include "Logger.h"
#include "LogStream.h"

#include <cbang/config.h>
#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/time/Time.h>
#include <cbang/time/Timer.h>
#include <cbang/io/NullStream.h>
#include <cbang/thread/SmartLock.h>
#include <cbang/util/RateSet.h>
#include <cbang/debug/Debugger.h>
#include <cbang/json/Sink.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/thread/ThreadLocalStorage.h>
#include <cbang/config/Options.h>
#include <cbang/event/Base.h>

#include <iostream>

using namespace std;
using namespace cb;

Mutex Logger::mutex;


Logger::Logger(Inaccessible) :
  rates(new RateSet), threadIDStorage(new ThreadLocalStorage<unsigned>),
  prefixStorage(new ThreadLocalStorage<string>) {
  setScreenStream(cout);

#ifdef _WIN32
  logCRLF  = true;
  logColor = false; // Windows does not handle ANSI color codes well
#endif
}


Logger::~Logger() {}


bool Logger::lock(double timeout) const {return mutex.lock(timeout);}
void Logger::unlock() const {mutex.unlock();}
bool Logger::tryLock() const {return mutex.tryLock();}


void Logger::initEvents(Event::Base &base) {
  (rotateEvent = base.newEvent(this, &Logger::rotate, 0))->activate();
  (dateEvent   = base.newEvent(this, &Logger::date,   0))->activate();
}


void Logger::addOptions(Options &options) {
  options.pushCategory("Logging");
  options.add("log", "Set log file.");
  options.addTarget("verbosity", verbosity, "Set logging level for INFO "
#ifdef DEBUG
                    "and DEBUG "
#endif
                    "messages.");
  options.addTarget("log-crlf", logCRLF, "Print carriage return and line feed "
                    "at end of log lines.");
#ifdef DEBUG
  options.addTarget("log-debug", logDebug, "Disable or enable debugging info.");
#endif
  options.addTarget("log-time", logTime,
                    "Print time information with log entries.");
  options.addTarget("log-date", logDate,
                    "Print date information with log entries.");
  options.addTarget("log-date-periodically", logDatePeriodically,
                    "Print date to log before new log entries if so many "
                    "seconds have passed since the last date was printed.");
  options.addTarget("log-short-level", logShortLevel, "Print shortened level "
                    "information with log entries.");
  options.addTarget("log-level", logLevel,
                    "Print level information with log entries.");
  options.addTarget("log-thread-prefix", logPrefix,
                    "Print thread prefixes, if set, with log entries.");
  options.addTarget("log-domain", logDomain,
                    "Print domain information with log entries.");
  options.addTarget("log-simple-domains", logSimpleDomains, "Remove any "
                    "leading directories and trailing file extensions from "
                    "domains so that source code file names can be easily used "
                    "as log domains.");
  options.add("log-domain-levels", 0, this, &Logger::domainLevelsAction,
              "Set log levels by domain.  Format is:\n"
              "\t<domain>[:i|d|t]:<level> ...\n"
              "Entries are separated by white-space and or commas.\n"
              "\ti - info\n"
#ifdef DEBUG
              "\td - debug\n"
#endif
#ifdef HAVE_CBANG_BACKTRACE
              "\tt - enable traces\n"
#endif
              "For example: server:i:3 module:6\n"
              "Set 'server' domain info messages to level 3 and 'module' info "
#ifdef DEBUG
              "and debug "
#endif
              "messages to level 6.  All other domains will follow "
              "the system wide log verbosity level.\n"
              "If <level> is negative it is relative to the system wide "
              "verbosity."
              )->setType(Option::TYPE_STRINGS);
  options.addTarget("log-thread-id", logThreadID, "Print id with log entries.");
  options.addTarget("log-header", logHeader, "Enable log message headers.");
  options.addTarget("log-no-info-header", logNoInfoHeader,
                    "Don't print 'INFO(#):' in header.");
  options.addTarget("log-color", logColor,
                    "Print log messages with ANSI color coding.");
  options.addTarget("log-to-screen", logToScreen, "Log to screen.");
  options.addTarget("log-truncate", logTrunc, "Truncate log file.");
  options.addTarget("log-rotate", logRotate, "Rotate log files on each run.");
  options.addTarget("log-rotate-dir", logRotateDir,
                    "Put rotated logs in this directory.");
  options.addTarget("log-rotate-compression", logRotateCompression,
                    "The type of compression to use when rotating log files.");
  options.addTarget("log-rotate-max", logRotateMax,
                    "Maximum number of rotated logs to keep.");
  options.addTarget("log-rotate-period", logRotatePeriod,
                    "Rotate log once every so many seconds.  No periodic "
                    "rotation is performed if zero.");
  options.popCategory();
}


int Logger::domainLevelsAction(Option &option) {
  setLogDomainLevels(option);
  return 0;
}


void Logger::setScreenStream(ostream &stream) {
  setScreenStream(SmartPointer<ostream>::Phony(&stream));
}


void Logger::setScreenStream(const SmartPointer<ostream> &stream) {
  screenStream = stream;
}


void Logger::startLogFile(const string &filename) {
  SmartLock lock(this);

  if (logRotate)
    try {
      if (logFile.isSet()) {
        logBar("Log Rotated", Time::now());
        logFile.release();
      }

      SystemUtilities::rotate(
        filename, logRotateDir, logRotateMax, logRotateCompression);
    } CATCH_ERROR;

  logFile = SystemUtilities::open(
    filename, ios::out | (logTrunc ? ios::trunc : ios::app));
  logFilename = filename;
  logBar("Log Started", Time::now());
  logFileCount = 0;
}


void Logger::setLogDomainLevels(const string &levels) {
  Option::strings_t entries;
  String::tokenize(levels, entries, ", \t\r\n");

  for (auto &entry: entries) {
    bool invalid = false;

    size_t pos = entry.find_last_of(':');

    if (!pos || pos == string::npos) invalid = true;
    else {
      int level = String::parseS32(entry.substr(pos + 1));

      size_t pos2 = entry.find_last_not_of("idt", pos - 1);
      if (pos2 && pos2 != string::npos && entry[pos2] == ':') {
        string name = entry.substr(0, pos2);

        while (++pos2 < pos)
          switch (entry[pos2]) {
          case 'i': infoDomainLevels[name] = level; break;
          case 'd': debugDomainLevels[name] = level; break;
#ifdef HAVE_CBANG_BACKTRACE
          case 't': domainTraces.insert(name); break;
#endif
          }

      } else {
        string name = entry.substr(0, pos);
        infoDomainLevels[name]  = level;
        debugDomainLevels[name] = level;
      }
    }

    if (invalid) THROW("Invalid log domain level entry '" << entry << "'");
  }
}


unsigned Logger::getHeaderWidth() const {
  return getHeader("", LOG_INFO_LEVEL(10)).size();
}


void Logger::setThreadID(unsigned id) {threadIDStorage->set(id);}


unsigned Logger::getThreadID() const {
  return threadIDStorage->isSet() ? threadIDStorage->get() : 0;
}


void Logger::setPrefix(const string &prefix) {
  prefixStorage->set(prefix);
}


string Logger::getPrefix() const {
  return prefixStorage->isSet() ? prefixStorage->get() : string();
}


string Logger::simplifyDomain(const string &domain) const {
  if (!logSimpleDomains) return domain;

  size_t pos = domain.find_last_of(SystemUtilities::path_separators);
  size_t start = pos == string::npos ? 0 : pos + 1;
  size_t end = domain.find_last_of('.');
  size_t len = end == string::npos ? end : end - start;

  return len ? domain.substr(start, len) : domain;
}


int Logger::domainVerbosity(const string &domain, int level) const {
  string dom = simplifyDomain(domain);

  if ((level & (LEVEL_DEBUG | LEVEL_INFO))) {
    auto &levels = level == LEVEL_DEBUG ? debugDomainLevels : infoDomainLevels;
    auto it = levels.find(dom);

    if (it != levels.end()) {
      int verbosity = it->second;

      // If verbosity < 0 then it is relative to Logger::verbosity
      return verbosity + (verbosity < 0 ? this->verbosity : 0);
    }
  }

  return verbosity;
}


char Logger::getLevelChar(int level) {
  level &= LEVEL_MASK;

  switch (level) {
  case LEVEL_ERROR:   return 'E';
  case LEVEL_WARNING: return 'W';
  case LEVEL_INFO:    return 'I';
  case LEVEL_DEBUG:   return 'D';
  default: THROW("Unknown log level " << level);
  }
}


string Logger::getHeader(const string &domain, int level) const {
  string header;

  if (!logHeader || level == LEVEL_RAW) return header;

  int verbosity = level >> 8;
  level &= LEVEL_MASK;

  // Thread ID
  if (logThreadID) {
    string idStr = String::printf("%0*u:", idWidth - 1, getThreadID());
    if (idWidth < idStr.length()) {
      lock();
      idWidth = idStr.length();
      unlock();
    }

    header += idStr;
  }

  // Date & Time
  if (logDate || logTime) {
    uint64_t now = Time::now(); // Must be the same time for both
    if (logDate) header += Time(now).toString("%Y-%m-%d:");
    if (logTime) header += Time(now).toString("%H:%M:%S:");
  }

  // Level
  if (logShortLevel) {
    header += string(1, getLevelChar(level));

    // Verbosity
    if (level >= LEVEL_INFO && verbosity) header += String(verbosity);
    else header += ' ';

    header += ':';

  } else if (logLevel && (!logNoInfoHeader || level != LEVEL_INFO)) {
    switch (level) {
    case LEVEL_ERROR:   header += "ERROR";   break;
    case LEVEL_WARNING: header += "WARNING"; break;
    case LEVEL_INFO:    header += "INFO";    break;
    case LEVEL_DEBUG:   header += "DEBUG";   break;
    default: THROW("Unknown log level " << level);
    }

    // Verbosity
    if (level >= LEVEL_INFO && verbosity)
      header += string("(") + String(verbosity) + ")";

    header += ':';
  }

  // Domain
  if (logDomain && domain != "") header += string(domain) + ':';

  // Thread Prefix
  if (logPrefix) header += getPrefix();

  return header;
}


const char *Logger::startColor(int level) const {
  if (!logColor) return "";

  switch (level & LEVEL_MASK) {
  case LEVEL_ERROR:   return "\033[91m";
  case LEVEL_WARNING: return "\033[93m";
  case LEVEL_DEBUG:   return "\033[92m";
  default: return "";
  }
}


const char *Logger::endColor(int level) const {
  if (!logColor) return "";

  switch (level & LEVEL_MASK) {
  case LEVEL_ERROR:
  case LEVEL_WARNING:
  case LEVEL_DEBUG:
    return "\033[0m";
  default: return "";
  }
}


void Logger::writeRates(JSON::Sink &sink) const {
  SmartLock lock(this);

  sink.beginDict();

  for (auto &p: *rates) {
    sink.insertDict(p.first);
    sink.insert("rate",  p.second.get());
    sink.insert("total", p.second.getTotal());

    auto it = rateMessages.find(p.first);
    if (it != rateMessages.end()) sink.insert("msg", it->second);
    sink.endDict();
  }

  sink.endDict();
}


bool Logger::enabled(const string &domain, int level) const {
  unsigned verbosity = level >> 8;
  level &= LEVEL_MASK;

  if (!logDebug && level == LEVEL_DEBUG) return false;

  if (level >= LEVEL_INFO && domainVerbosity(domain, level) < (int)verbosity)
    return false;

  // All other log levels are always enabled
  return true;
}


Logger::LogStream Logger::createStream(const string &_domain, int level,
                                       const string &_prefix,
                                       const char *filename, int line) {
  string domain = simplifyDomain(_domain);

  if (!enabled(domain, level)) return new NullStream<>;

  SmartLock lock(this);

  string rateKey;
  if ((level & logRates) && rates.isSet()) {
    rateKey = SSTR(getLevelChar(level) << ':' << filename << ':' << line);
    rates->event(rateKey);
  }

  string prefix = startColor(level) + getHeader(domain, level) + _prefix;
  string suffix = endColor(level);
  string trailer;

#ifdef HAVE_CBANG_BACKTRACE
  if (domainTraces.find(domain) != domainTraces.end()) {
    StackTrace trace;
    Debugger::instance().getStackTrace(trace);
    trace.erase(trace.begin(), trace.begin() + 3);
    trailer = SSTR('\n' << trace);
  }
#endif

  return new cb::LogStream(
    new cb::LogDevice::impl(prefix, suffix, trailer, rateKey));
}


void Logger::rateMessage(const string &key, const string &msg) {
  rateMessages[key] = msg;
}


void Logger::logBar(const string &msg, uint64_t ts) const {
  *logFile << String::bar(msg + " " + Time(ts).toString())
           << (logCRLF ? "\r\n" : "\n") << std::flush;
}


void Logger::write(const char *s, streamsize n) {
  if (!logFile.isNull()) {logFile->write(s, n); logFileCount++;}
  if (logToScreen && !screenStream.isNull()) screenStream->write(s, n);
  for (auto &l: listeners) TRY_CATCH_ERROR(l->write(s, n));
  flush();
}


void Logger::write(const string &s) {write(s.data(), s.length());}


bool Logger::flush() {
  if (!logFile.isNull()) logFile->flush();
  if (logToScreen && !screenStream.isNull()) screenStream->flush();
  return true;
}


void Logger::rotate() {
  if (firstRotate) firstRotate = false;
  else if (logFileCount) startLogFile(logFilename);

  if (logRotate && logRotatePeriod)
    rotateEvent->next(logRotatePeriod);
}


void Logger::date() {
  if (firstDate) firstDate = false;
  else {
    SmartLock lock(this);
    write(String::bar(Time().toString("Date: %Y-%m-%d")) +
          (logCRLF ? "\r\n" : "\n"));
  }

  if (logDatePeriodically) dateEvent->next(logDatePeriodically);
}
