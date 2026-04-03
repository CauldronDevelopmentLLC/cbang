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

// Tests for LogListener and LogLineListener level-awareness
// Returns 0 if all selected tests pass, 1 otherwise.
// Usage:
//   ./loglistener            -- run all tests
//   ./loglistener <name>     -- run one named test
//   ./loglistener --list     -- list all test names

#include <cbang/log/Logger.h>
#include <cbang/log/LogLineListener.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace cb;

// ---------------------------------------------------------------------------
// Test framework
// ---------------------------------------------------------------------------
namespace {
int g_pass = 0;
int g_fail = 0;
}

#define CHECK(cond, msg)                                                  \
  do {                                                                    \
    if (cond) { ++g_pass; }                                               \
    else {                                                                \
      ++g_fail;                                                           \
      std::cerr << "  FAIL [line " << __LINE__ << "]: " << msg << "\n";  \
    }                                                                     \
  } while(0)


// ---------------------------------------------------------------------------
// Test LogListener that records level from 3-arg write()
// ---------------------------------------------------------------------------
class TestLogListener : public LogListener {
public:
  int lastLevel = -1;
  unsigned totalBytes = 0;
  int writeCount = 0;
  int writeLevelCount = 0;

  void write(const char *s, unsigned n) override {
    totalBytes += n;
    writeCount++;
  }

  void write(const char *s, unsigned n, int level) override {
    totalBytes += n;
    lastLevel = level;
    writeLevelCount++;
  }
};


// ---------------------------------------------------------------------------
// Test LogLineListener that records level from writeln()
// ---------------------------------------------------------------------------
class TestLogLineListener : public LogLineListener {
public:
  int lastLevel = -1;
  int lineCount = 0;
  int levelLineCount = 0;
  std::string lastLine;

  void writeln(const char *s) override {
    lastLine = s;
    lineCount++;
  }

  void writeln(const char *s, int level) override {
    lastLine = s;
    lastLevel = level;
    levelLineCount++;
  }
};


// ---------------------------------------------------------------------------
// Helper: suppress log output during tests
// ---------------------------------------------------------------------------
struct LogGuard {
  LogGuard() {
    Logger::instance().setLogToScreen(false);
  }
  ~LogGuard() {
    Logger::instance().setLogToScreen(true);
  }
};


// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

bool test_listener_receives_level() {
  LogGuard guard;
  auto listener = SmartPtr(new TestLogListener);
  Logger::instance().addListener(listener);

  LOG_ERROR("test error");

  Logger::instance().removeListener(listener);

  CHECK(listener->writeLevelCount > 0,
        "3-arg write() was called at least once");
  CHECK((listener->lastLevel & Logger::LEVEL_MASK) == Logger::LEVEL_ERROR,
        "level is LEVEL_ERROR");
  return true;
}


bool test_listener_warning_level() {
  LogGuard guard;
  auto listener = SmartPtr(new TestLogListener);
  Logger::instance().addListener(listener);

  LOG_WARNING("test warning");

  Logger::instance().removeListener(listener);

  CHECK(listener->writeLevelCount > 0,
        "3-arg write() was called");
  CHECK((listener->lastLevel & Logger::LEVEL_MASK) == Logger::LEVEL_WARNING,
        "level is LEVEL_WARNING");
  return true;
}


bool test_listener_info_level() {
  LogGuard guard;
  auto listener = SmartPtr(new TestLogListener);
  Logger::instance().addListener(listener);

  LOG_INFO(1, "test info");

  Logger::instance().removeListener(listener);

  CHECK(listener->writeLevelCount > 0,
        "3-arg write() was called");
  CHECK((listener->lastLevel & Logger::LEVEL_MASK) == Logger::LEVEL_INFO,
        "level is LEVEL_INFO");
  return true;
}


bool test_listener_default_delegates_to_write() {
  // A listener that only overrides 2-arg write() should still work
  // via the default 3-arg impl in LogListener base
  struct BasicListener : public LogListener {
    int callCount = 0;
    void write(const char *s, unsigned n) override {callCount++;}
  };

  LogGuard guard;
  auto listener = SmartPtr(new BasicListener);
  Logger::instance().addListener(listener);

  LOG_ERROR("test");

  Logger::instance().removeListener(listener);

  CHECK(listener->callCount > 0,
        "2-arg write() called via default 3-arg delegation");
  return true;
}


bool test_line_listener_receives_level() {
  LogGuard guard;
  auto listener = SmartPtr(new TestLogLineListener);
  Logger::instance().addListener(listener);

  LOG_ERROR("line error");

  Logger::instance().removeListener(listener);

  CHECK(listener->levelLineCount > 0,
        "2-arg writeln() was called");
  CHECK((listener->lastLevel & Logger::LEVEL_MASK) == Logger::LEVEL_ERROR,
        "writeln level is LEVEL_ERROR");
  return true;
}


bool test_line_listener_warning() {
  LogGuard guard;
  auto listener = SmartPtr(new TestLogLineListener);
  Logger::instance().addListener(listener);

  LOG_WARNING("line warning");

  Logger::instance().removeListener(listener);

  CHECK(listener->levelLineCount > 0,
        "2-arg writeln() was called");
  CHECK((listener->lastLevel & Logger::LEVEL_MASK) == Logger::LEVEL_WARNING,
        "writeln level is LEVEL_WARNING");
  return true;
}


bool test_multiple_log_calls_track_levels() {
  LogGuard guard;
  auto listener = SmartPtr(new TestLogListener);
  Logger::instance().addListener(listener);

  LOG_ERROR("err1");
  CHECK((listener->lastLevel & Logger::LEVEL_MASK) == Logger::LEVEL_ERROR,
        "first call is error");

  LOG_WARNING("warn1");
  CHECK((listener->lastLevel & Logger::LEVEL_MASK) == Logger::LEVEL_WARNING,
        "second call is warning");

  LOG_INFO(1, "info1");
  CHECK((listener->lastLevel & Logger::LEVEL_MASK) == Logger::LEVEL_INFO,
        "third call is info");

  Logger::instance().removeListener(listener);
  return true;
}


bool test_counting_errors_and_warnings() {
  // Simulates how LogTracker counts errors and warnings
  struct CountingListener : public LogLineListener {
    uint64_t errorCount   = 0;
    uint64_t warningCount = 0;

    void writeln(const char *s) override {}
    void writeln(const char *s, int level) override {
      int masked = level & Logger::LEVEL_MASK;
      if (masked == Logger::LEVEL_ERROR)   errorCount++;
      if (masked == Logger::LEVEL_WARNING) warningCount++;
    }
  };

  LogGuard guard;
  auto listener = SmartPtr(new CountingListener);
  Logger::instance().addListener(listener);

  LOG_ERROR("err1");
  LOG_ERROR("err2");
  LOG_WARNING("warn1");
  LOG_INFO(1, "info1");
  LOG_WARNING("warn2");
  LOG_ERROR("err3");

  Logger::instance().removeListener(listener);

  CHECK(listener->errorCount == 3,   "3 errors counted");
  CHECK(listener->warningCount == 2, "2 warnings counted");
  return true;
}


bool test_info_not_counted_as_error_or_warning() {
  struct CountingListener : public LogLineListener {
    uint64_t errorCount   = 0;
    uint64_t warningCount = 0;

    void writeln(const char *s) override {}
    void writeln(const char *s, int level) override {
      int masked = level & Logger::LEVEL_MASK;
      if (masked == Logger::LEVEL_ERROR)   errorCount++;
      if (masked == Logger::LEVEL_WARNING) warningCount++;
    }
  };

  LogGuard guard;
  auto listener = SmartPtr(new CountingListener);
  Logger::instance().addListener(listener);

  LOG_INFO(1, "info1");
  LOG_INFO(2, "info2");
  LOG_INFO(3, "info3");

  Logger::instance().removeListener(listener);

  CHECK(listener->errorCount == 0,   "no errors from INFO");
  CHECK(listener->warningCount == 0, "no warnings from INFO");
  return true;
}


bool test_line_listener_default_delegates_to_writeln() {
  // A LogLineListener that only overrides 1-arg writeln() should still work
  struct BasicLineListener : public LogLineListener {
    int callCount = 0;
    void writeln(const char *s) override {callCount++;}
  };

  LogGuard guard;
  auto listener = SmartPtr(new BasicLineListener);
  Logger::instance().addListener(listener);

  LOG_ERROR("test");

  Logger::instance().removeListener(listener);

  CHECK(listener->callCount > 0,
        "1-arg writeln() called via default 2-arg delegation");
  return true;
}


// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------
using TestFn = std::function<bool()>;

namespace {
std::map<std::string, TestFn> &registry() {
  static std::map<std::string, TestFn> r; return r;
}
#define REGISTER(fn) registry()[#fn] = fn

void registerAll() {
  REGISTER(test_listener_receives_level);
  REGISTER(test_listener_warning_level);
  REGISTER(test_listener_info_level);
  REGISTER(test_listener_default_delegates_to_write);
  REGISTER(test_line_listener_receives_level);
  REGISTER(test_line_listener_warning);
  REGISTER(test_multiple_log_calls_track_levels);
  REGISTER(test_counting_errors_and_warnings);
  REGISTER(test_info_not_counted_as_error_or_warning);
  REGISTER(test_line_listener_default_delegates_to_writeln);
}
}


// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  registerAll();

  if (argc == 2 && std::strcmp(argv[1], "--list") == 0) {
    for (auto &kv : registry()) std::cout << kv.first << "\n";
    return 0;
  }

  std::vector<std::string> toRun;
  if (argc >= 2) {
    for (int i = 1; i < argc; ++i) {
      if (registry().find(argv[i]) == registry().end()) {
        std::cerr << "Unknown test: " << argv[i] << "\n";
        return 1;
      }
      toRun.push_back(argv[i]);
    }

  } else {
    for (auto &kv : registry()) toRun.push_back(kv.first);
  }

  int failed_tests = 0;
  for (auto &name : toRun) {
    int before = g_fail;
    bool ok = false;
    try { ok = registry()[name](); }
    catch (const std::exception &e) {
      std::cerr << "  EXCEPTION in " << name << ": " << e.what() << "\n";
    } catch (...) {
      std::cerr << "  UNKNOWN EXCEPTION in " << name << "\n";
    }
    if (ok && g_fail == before)
      std::cout << "PASS: " << name << "\n";
    else {
      std::cout << "FAIL: " << name << "\n";
      ++failed_tests;
    }
  }

  std::cout << "\n" << g_pass << " checks passed, "
            << g_fail << " checks failed, "
            << failed_tests << " tests failed.\n";
  return failed_tests ? 1 : 0;
}
