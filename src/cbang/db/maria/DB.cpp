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

#include "DB.h"

#include <cbang/String.h>
#include <cbang/Exception.h>
#include <cbang/time/Time.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

#include <mysql/mysql.h>

#include <cstring>

#define RAISE_ERROR(msg) raiseError(SSTR(msg), false)
#define RAISE_DB_ERROR(msg) raiseError(SSTR(msg), true)

using namespace std;
using namespace cb;
using namespace cb::MariaDB;


namespace cb {
  namespace MariaDB {
    // Bound output columns for the binary protocol.  Every column is bound as
    // a string so fetched data matches what the text protocol produced and the
    // existing accessors (getString/getDouble/...) keep working.
    struct Binding {
      vector<MYSQL_BIND>   binds;
      vector<vector<char>> data;
      vector<unsigned long> length;
      vector<my_bool>      isNull;
      vector<my_bool>      error;

      // Bound input parameters
      vector<string>        params;
      vector<my_bool>       paramNull;
      vector<MYSQL_BIND>    paramBinds;
      vector<unsigned long> paramLength;

      void resize(unsigned n) {
        binds .assign(n, MYSQL_BIND());
        data  .assign(n, vector<char>());
        length.assign(n, 0);
        isNull.assign(n, 0);
        error .assign(n, 0);
      }
    };


    // Result columns are fetched as text.  mysql_stmt_fetch() runs against a
    // zero-length buffer purely to report each column's length, then
    // processFetch() pulls each value with mysql_stmt_fetch_column() into a
    // buffer sized to that length -- but never below MIN_RESULT_BUFFER.  That
    // floor is what fixes FLOAT/DOUBLE: libmariadb renders them to text with
    // ma_gcvt() using buffer_length as the field width, and the zero-length
    // probe reports their length as 0, so without a floor the re-fetch would
    // ask for a zero-width conversion and get an empty string.  512 comfortably
    // exceeds MAX_DOUBLE_STRING_REP_LENGTH (~331).  BLOB/TEXT columns report
    // their true (larger) length and are sized to it exactly, so there is no
    // upper bound and large values are never truncated.
    static const unsigned long MIN_RESULT_BUFFER = 512;
  }
}


DB::DB(st_mysql *db) :
  db(db ? db : mysql_init(0)), nonBlocking(false), connected(false),
  status(0), continueFunc(0) {
  LOG_DEBUG(5, CBANG_FUNC << "()");
  if (!this->db) RAISE_DB_ERROR("Failed to create MariaDB");
  binding = new Binding;
}


DB::~DB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");
  freeMeta();
  if (stmt) mysql_stmt_close(stmt);
  if (db) mysql_close(db);
  delete binding;
}


void DB::setInitCommand(const string &cmd) {
  if (mysql_options(db, MYSQL_INIT_COMMAND, cmd.c_str()))
    RAISE_DB_ERROR("Failed to set MariaDB init command: " << cmd);
}


void DB::enableCompression() {
  if (mysql_options(db, MYSQL_OPT_COMPRESS, 0))
    RAISE_DB_ERROR("Failed to enable MariaDB compress");
}


void DB::setConnectTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, &secs))
    RAISE_DB_ERROR("Failed to set MariaDB connect timeout to " << secs);
}


void DB::setLocalInFile(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_OPT_LOCAL_INFILE, &x))
    RAISE_DB_ERROR("Failed to " << (enable ? "enable" : "disable")
                << " MariaD local infile");
}


void DB::enableNamedPipe() {
  if (mysql_options(db, MYSQL_OPT_NAMED_PIPE, 0))
    RAISE_DB_ERROR("Failed to enable MariaDB named pipe");
}


void DB::setProtocol(protocol_t protocol) {
  mysql_protocol_type type;
  switch (protocol) {
  case PROTOCOL_TCP: type = MYSQL_PROTOCOL_TCP; break;
  case PROTOCOL_SOCKET: type = MYSQL_PROTOCOL_SOCKET; break;
  case PROTOCOL_PIPE: type = MYSQL_PROTOCOL_PIPE; break;
  default: RAISE_ERROR("Invalid protocol " << protocol);
  }

  if (mysql_options(db, MYSQL_OPT_PROTOCOL, &type))
    RAISE_DB_ERROR("Failed to set MariaDB protocol to " << protocol);
}


void DB::setReconnect(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_OPT_RECONNECT, &x))
    RAISE_DB_ERROR("Failed to " << (enable ? "enable" : "disable")
                   << "MariaDB auto reconnect");
}


void DB::setReadTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_READ_TIMEOUT, &secs))
    RAISE_DB_ERROR("Failed to set MariaDB read timeout to " << secs);
}


void DB::setWriteTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_WRITE_TIMEOUT, &secs))
    RAISE_DB_ERROR("Failed to set MariaDB write timeout to " << secs);
}


void DB::setDefaultFile(const string &path) {
  if (mysql_options(db, MYSQL_READ_DEFAULT_FILE, path.c_str()))
    RAISE_DB_ERROR("Failed to set MariaDB default type to " << path);
}


void DB::readDefaultGroup(const string &path) {
  if (mysql_options(db, MYSQL_READ_DEFAULT_GROUP, path.c_str()))
    RAISE_DB_ERROR("Failed to read MariaDB default group file " << path);
}


void DB::setReportDataTruncation(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_REPORT_DATA_TRUNCATION, &x))
    RAISE_DB_ERROR("Failed to" << (enable ? "enable" : "disable")
                << " MariaDB data truncation reporting.");
}


void DB::setCharacterSet(const string &name) {
  if (mysql_options(db, MYSQL_SET_CHARSET_NAME, name.c_str()))
    RAISE_DB_ERROR("Failed to set MariaDB character set to " << name);
}


void DB::enableNonBlocking() {
  if (mysql_options(db, MYSQL_OPT_NONBLOCK, 0))
    RAISE_DB_ERROR("Failed to set MariaDB to non-blocking mode");
  nonBlocking = true;
}


void DB::connect(const string &host, const string &user, const string &password,
                 const string &dbName, unsigned port, const string &socketName,
                 flags_t flags) {
  assertNotPending();
  MYSQL *db = mysql_real_connect(
    this->db, host.c_str(), user.c_str(), password.c_str(), dbName.c_str(),
    port, socketName.empty() ? 0 : socketName.c_str(), flags);

  if (!db) RAISE_DB_ERROR("Failed to connect");
  connected = true;
}


bool DB::connectNB(const string &host, const string &user,
                   const string &password, const string &dbName, unsigned port,
                   const string &socketName, flags_t flags) {
  LOG_DEBUG(5, CBANG_FUNC << "(host=" << host << ", user=" << user
    << ", db=" << dbName << ", port=" << port
    << ", socketName=" << socketName << ", flags=" << flags << ")");

  assertNotPending();
  assertNonBlocking();

  MYSQL *db = 0;
  status = mysql_real_connect_start(
    &db, this->db, host.c_str(), user.c_str(), password.c_str(),
    dbName.c_str(), port, socketName.empty() ? 0 : socketName.c_str(), flags);

  if (status) {
    continueFunc = &DB::connectContinue;
    return false;
  }

  if (!db) RAISE_DB_ERROR("Failed to connect");
  connected = true;

  return true;
}


void DB::resetConnection() {
  assertNotPending();

  if (mysql_reset_connection(db))
    RAISE_DB_ERROR("Failed to reset DB connection");
}


bool DB::resetConnectionNB() {
  assertNotPending();

  int ret = 0;
  mysql_reset_connection_start(&ret, db);

  if (status) {
    continueFunc = &DB::resetConnectionContinue;
    return false;
  }

  if (ret) RAISE_DB_ERROR("Failed to reset DB connection");

  return true;
}


void DB::changeUser(const string &user, const string &password,
                    const string &dbName) {
  assertNotPending();
  my_bool ret = mysql_change_user(
    db, user.c_str(), password.c_str(), dbName.c_str());

  if (ret) RAISE_DB_ERROR("Failed to change DB user");
}


bool DB::changeUserNB(const string &user, const string &password,
                      const string &dbName) {
  LOG_DEBUG(5, CBANG_FUNC << "(user=" << user << " db=" << dbName << ")");

  assertNotPending();
  assertNonBlocking();

  my_bool ret = false;
  status = mysql_change_user_start(
    &ret, db, user.c_str(), password.c_str(), dbName.c_str());

  if (status) {
    continueFunc = &DB::changeUserContinue;
    return false;
  }

  if (ret) RAISE_DB_ERROR("Failed to change DB user");

  return true;
}


bool DB::ping() {return !mysql_ping(db);}


bool DB::pingNB() {
  int ret = 0;
  status = mysql_ping_start(&ret, db);

  if (status) {
    continueFunc = &DB::pingContinue;
    return false;
  }

  connected = !ret;

  return true;
}


void DB::close() {
  if (!connected) return;

  if (db) {
    mysql_close(db);
    db = mysql_init(0);
  }
  connected = false;
}


bool DB::closeNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();

  status = mysql_close_start(db);
  if (status) {
    continueFunc = &DB::closeContinue;
    return false;
  }

  db = mysql_init(0);
  connected = false;
  return true;
}


void DB::use(const string &dbName) {
  assertConnected();
  assertNotPending();

  if (mysql_select_db(db, dbName.c_str()))
    RAISE_DB_ERROR("Failed to select DB");
}


bool DB::useNB(const string &dbName) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();

  int ret = 0;
  status = mysql_select_db_start(&ret, db, dbName.c_str());

  if (status) {
    continueFunc = &DB::useContinue;
    return false;
  }

  if (ret) RAISE_DB_ERROR("Failed to select DB");

  return true;
}


bool DB::queryNB(const string &s, const vector<JSON::ValuePtr> &params) {
  assertConnected();
  assertNotPending();
  assertNonBlocking();

  if (!stmt && !(stmt = mysql_stmt_init(db)))
    RAISE_DB_ERROR("Failed to allocate statement");

  rowReady = false;

  // Convert params to bound buffers; nulls bind NULL, booleans bind 1/0
  binding->params.clear();
  binding->paramNull.clear();
  for (auto &p: params) {
    bool null = p.isNull() || p->isNull() || p->isUndefined();
    binding->paramNull.push_back(null);
    binding->params.push_back(
      null ? string() :
      p->isBoolean() ? string(p->getBoolean() ? "1" : "0") : p->asString());
  }

  int ret = 0;
  status = mysql_stmt_prepare_start(&ret, stmt, s.data(), s.length());
  LOG_DEBUG(5, CBANG_FUNC << "() status=" << status);

  if (status) {continueFunc = &DB::prepareContinue; return false;}
  if (ret) RAISE_DB_ERROR("Prepare failed");

  return startExecute();
}


bool DB::startExecute() {
  auto &params = binding->params;
  unsigned count = mysql_stmt_param_count(stmt);
  if (count != params.size())
    RAISE_ERROR("SQL expects " << count << " bound parameters but "
                << params.size() << " were provided");

  if (!params.empty()) {
    auto &binds   = binding->paramBinds;
    auto &lengths = binding->paramLength;

    binds.assign(params.size(), MYSQL_BIND());
    lengths.assign(params.size(), 0);

    for (unsigned i = 0; i < params.size(); i++) {
      lengths[i]             = params[i].length();
      binds[i].buffer_type   =
        binding->paramNull[i] ? MYSQL_TYPE_NULL : MYSQL_TYPE_BLOB;
      binds[i].buffer        = (void *)params[i].data();
      binds[i].buffer_length = params[i].length();
      binds[i].length        = &lengths[i];
    }

    if (mysql_stmt_bind_param(stmt, binds.data()))
      RAISE_DB_ERROR("Failed to bind parameters");
  }

  int ret = 0;
  status = mysql_stmt_execute_start(&ret, stmt);

  if (status) {continueFunc = &DB::executeContinue; return false;}
  if (ret) RAISE_DB_ERROR("Execute failed");

  return true;
}


void DB::freeMeta() {if (meta) {mysql_free_result(meta); meta = 0;}}


void DB::setupResultBind() {
  unsigned n = mysql_num_fields(meta);
  binding->resize(n);

  for (unsigned i = 0; i < n; i++) {
    // Bind a zero-length buffer: mysql_stmt_fetch() then reports each column's
    // length without copying, and processFetch() pulls the value into a
    // right-sized buffer.  The statement keeps its own copy of these binds, so
    // resizing our buffers later does not disturb it (no rebind needed).
    MYSQL_BIND &b = binding->binds[i];
    b.buffer_type   = MYSQL_TYPE_STRING; // fetch every column as text
    b.buffer        = 0;
    b.buffer_length = 0;
    b.length        = &binding->length[i];
    b.is_null       = &binding->isNull[i];
    b.error         = &binding->error[i];
  }

  if (mysql_stmt_bind_result(stmt, binding->binds.data()))
    RAISE_DB_ERROR("Failed to bind result");
}


bool DB::storeResultNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();

  rowReady = false;
  freeMeta();

  meta = mysql_stmt_result_metadata(stmt);
  if (!meta) return true; // No result set (e.g. INSERT/UPDATE/OK)

  setupResultBind();

  int ret = 0;
  status = mysql_stmt_store_result_start(&ret, stmt);
  if (status) {continueFunc = &DB::storeResultContinue; return false;}
  if (ret) RAISE_DB_ERROR("Failed to store result");

  return true;
}


bool DB::haveResult() const {return meta;}


bool DB::nextResultNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();

  int ret = 0;
  status = mysql_stmt_next_result_start(&ret, stmt);
  if (status) {continueFunc = &DB::nextResultContinue; return false;}

  if (0 < ret) RAISE_DB_ERROR("Failed to get next result");

  return true;
}


bool DB::moreResults() const {
  assertConnected();
  return mysql_stmt_more_results(stmt);
}


bool DB::freeResultNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertNotPending();
  assertNonBlocking();

  rowReady = false;
  freeMeta();

  my_bool ret = 0;
  status = mysql_stmt_free_result_start(&ret, stmt);
  if (status) {continueFunc = &DB::freeResultContinue; return false;}

  return true;
}


uint64_t DB::getRowCount() const {return stmt ? mysql_stmt_num_rows(stmt) : 0;}


uint64_t DB::getAffectedRowCount() const {
  return stmt ? mysql_stmt_affected_rows(stmt) : 0;
}


unsigned DB::getFieldCount() const {return meta ? mysql_num_fields(meta) : 0;}


bool DB::fetchRowNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertNotPending();
  assertNonBlocking();
  assertHaveResult();

  status = mysql_stmt_fetch_start(&fetchRet, stmt);
  if (status) {continueFunc = &DB::fetchRowContinue; return false;}

  processFetch();
  return true;
}


void DB::processFetch() {
  if (fetchRet == MYSQL_NO_DATA) {rowReady = false; return;}
  if (fetchRet == 1) RAISE_DB_ERROR("Fetch failed");

  // fetchRet is 0 or MYSQL_DATA_TRUNCATED.  The initial bind used a zero-length
  // buffer, so mysql_stmt_fetch() reported each column's length without
  // copying; pull every non-null column into its own buffer.  The buffer is at
  // least MIN_RESULT_BUFFER bytes so FLOAT/DOUBLE columns -- which libmariadb
  // renders to text with a field width equal to buffer_length, and which the
  // probe reports as length 0 -- come out in full rather than empty.  Larger
  // values (BLOB/TEXT) report their true length and are sized to it exactly.
  // mysql_stmt_fetch_column() takes the buffer explicitly, so the statement's
  // own (zero-length) bound copy is untouched and needs no rebind between rows.
  unsigned n = getFieldCount();
  for (unsigned i = 0; i < n; i++) {
    if (binding->isNull[i]) continue;

    unsigned long need = binding->length[i];
    if (need < MIN_RESULT_BUFFER) need = MIN_RESULT_BUFFER;
    if (binding->data[i].size() < need) binding->data[i].resize(need);

    MYSQL_BIND &b   = binding->binds[i];
    b.buffer        = binding->data[i].data();
    b.buffer_length = binding->data[i].size();
    if (mysql_stmt_fetch_column(stmt, &b, i, 0))
      RAISE_DB_ERROR("Failed to fetch column");
  }

  rowReady = true;
}


bool DB::haveRow() const {return rowReady;}


void DB::appendRow(JSON::Sink &sink, int first, int count) const {
  for (unsigned i = first; i < getFieldCount() && count; i++, count--) {
    sink.beginAppend();
    writeField(sink, i);
  }
}


void DB::insertRow(JSON::Sink &sink, int first, int count,
                   bool withNulls) const {

  for (unsigned i = first; i < getFieldCount() && count; i++, count--) {
    if (!withNulls && getNull(i)) continue;
    sink.beginInsert(getField(i).getName());
    writeField(sink, i);
  }
}


void DB::writeRowList(JSON::Sink &sink, int first, int count) const {
  sink.beginList();
  appendRow(sink, first, count);
  sink.endList();
}


void DB::writeRowDict(JSON::Sink &sink, int first, int count,
                      bool withNulls) const {
  sink.beginDict();
  insertRow(sink, first, count, withNulls);
  sink.endDict();
}


SmartPointer<JSON::Value> DB::getRowList(int first, int last) const {
  JSON::Builder builder;
  writeRowList(builder, first, last);
  return builder.getRoot();
}


SmartPointer<JSON::Value> DB::getRowDict(int first, int last,
                                         bool withNulls) const {
  JSON::Builder builder;
  writeRowDict(builder, first, last, withNulls);
  return builder.getRoot();
}


Field DB::getField(unsigned i) const {
  assertInFieldRange(i);
  return &mysql_fetch_fields(meta)[i];
}


Field::type_t DB::getType(unsigned i) const {
  return getField(i).getType();
}


unsigned DB::getLength(unsigned i) const {
  assertInFieldRange(i);
  return binding->length[i];
}


const char *DB::getData(unsigned i) const {
  assertInFieldRange(i);
  return binding->isNull[i] ? 0 : binding->data[i].data();
}


void DB::writeField(JSON::Sink &sink, unsigned i) const {
  if (getNull(i)) sink.writeNull();
  else {
    Field field = getField(i);

    // TODO write integer types as integers
    if (field.isNumber()) sink.write(getDouble(i));
    else sink.write(getString(i));
  }
}


void DB::insertField(JSON::Value &value, unsigned i) const {
  value.insert(getField(i).getName(), getJSON(i));
}


JSON::ValuePtr DB::getJSON(unsigned i) const {
  if (getNull(i)) return JSON::Factory().createNull();

  Field field = getField(i);

  // TODO write integer types as integers
  if (field.isNumber()) return JSON::Factory().create(getDouble(i));
  return JSON::Factory().create(getString(i));
}



bool DB::getNull(unsigned i) const {
  assertInFieldRange(i);
  return binding->isNull[i];
}


string DB::getString(unsigned i) const {
  if (getNull(i)) return "";
  return string(binding->data[i].data(), binding->length[i]);
}


bool DB::getBoolean(unsigned i) const {return String::parseBool(getString(i));}


double DB::getDouble(unsigned i) const {
  if (!getField(i).isNumber())
    RAISE_ERROR("Field " << i << " is not a number");
  return String::parseDouble(getString(i));
}


uint32_t DB::getU32(unsigned i) const {
  if (!getField(i).isInteger())
    RAISE_ERROR("Field " << i << " is not an integer");
  return String::parseU32(getString(i));
}


int32_t DB::getS32(unsigned i) const {
  if (!getField(i).isInteger())
    RAISE_ERROR("Field " << i << " is not an integer");
  return String::parseS32(getString(i));
}


uint64_t DB::getU64(unsigned i) const {
  if (!getField(i).isInteger())
    RAISE_ERROR("Field " << i << " is not an integer");
  return String::parseU64(getString(i));
}


int64_t DB::getS64(unsigned i) const {
  if (!getField(i).isInteger())
    RAISE_ERROR("Field " << i << " is not an integer");
  return String::parseS64(getString(i));
}


uint64_t DB::getBit(unsigned i) const {
  if (getType(i) != Field::TYPE_BIT)
    RAISE_ERROR("Field " << i << " is not bit");

  uint64_t x = 0;
  for (char c: getString(i)) {
    x <<= 1;
    if (c == '1') x |= 1;
  }

  return x;
}


void DB::getSet(unsigned i, set<string> &s) const {
  if (getType(i) != Field::TYPE_SET)
    RAISE_ERROR("Field " << i << " is not a set");

  string v = getString(i);
  size_t start = 0;
  for (size_t end = 0; end <= v.size(); end++)
    if (end == v.size() || v[end] == ',') {
      if (start < end) s.insert(v.substr(start, end - start));
      start = end + 1;
    }
}


double DB::getTime(unsigned i) const {
  assertInFieldRange(i);

  string time = getString(i);

  // Parse decimal part
  double decimal = 0;
  auto dot = time.find('.');
  if (dot != string::npos) {
    decimal = String::parseDouble(time.substr(dot));
    time = time.substr(0, dot);
  }

  switch (getType(i)) {
  case Field::TYPE_YEAR:
    if (time.length() == 2) return decimal + Time::parse(time, "%y");
    return decimal + Time::parse(time, "%Y");

  case Field::TYPE_DATE:
    return decimal + Time::parse(time, "%Y-%m-%d");

  case Field::TYPE_TIME:
    return decimal + Time::parse(time, "%H%M%S");

  case Field::TYPE_TIMESTAMP:
  case Field::TYPE_DATETIME:
    return decimal + Time::parse(time, "%Y-%m-%d %H%M%S");

  default:
    RAISE_ERROR("Invalid time type");
    return 0;
  }
}


string DB::rowToString() const {
  ostringstream str;

  for (unsigned i = 0; i < getFieldCount(); i++) {
    if (i) str << ", ";
    str << getString(i);
  }

  return str.str();
}


string DB::getInfo() const {return mysql_info(db);}

const char *DB::getSQLState() const {
  return stmt ? mysql_stmt_sqlstate(stmt) : mysql_sqlstate(db);
}

bool     DB::hasError()       const {return getErrorNumber();}
string   DB::getError()       const {
  return stmt ? mysql_stmt_error(stmt) : mysql_error(db);
}
unsigned DB::getErrorNumber() const {
  return stmt ? mysql_stmt_errno(stmt) : mysql_errno(db);
}


void DB::raiseError(const string &msg, bool withDBError) const {
  if (!withDBError || db) THROW("MariaDB: " << msg << ": " << getError());
  else THROW("MariaDB: " << msg);
}


unsigned DB::getWarningCount() const {return mysql_warning_count(db);}


void DB::assertConnected() const {
  if (!connected) RAISE_ERROR("Not connected");
}


void DB::assertPending() const {
  if (!nonBlocking || !status)
    RAISE_ERROR("Non-blocking call not pending");
}


void DB::assertNotPending() const {
  if (status) RAISE_ERROR("Non-blocking call still pending");
}


void DB::assertNonBlocking() const {
  if (!nonBlocking) RAISE_ERROR("Connection is not in nonBlocking mode");
}


void DB::assertHaveResult() const {
  if (!haveResult()) RAISE_ERROR("Don't have result, must call query() and "
                                 "useResult() or storeResult()");
}


void DB::assertNotHaveResult() const {
  if (haveResult())
    RAISE_ERROR("Already have result, must call freeResult()");
}


void DB::assertHaveRow() const {
  if (!haveRow())
    RAISE_ERROR("Don't have row, must call fetchRow()");
}


void DB::assertInFieldRange(unsigned i) const {
  if (getFieldCount() <= 0)
    RAISE_ERROR("Out of field range " << i);
}


bool DB::continueNB(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertPending();
  if (!continueFunc) RAISE_ERROR("Continue function not set");
  bool ret = (this->*continueFunc)(ready);
  if (ret) continueFunc = 0;
  return ret;
}


bool DB::waitRead()    const {return status & MYSQL_WAIT_READ;}
bool DB::waitWrite()   const {return status & MYSQL_WAIT_WRITE;}
bool DB::waitExcept()  const {return status & MYSQL_WAIT_EXCEPT;}
bool DB::waitTimeout() const {return status & MYSQL_WAIT_TIMEOUT;}
int  DB::getSocket()   const {return mysql_get_socket(db);}


double DB::getTimeout() const {
  return (double)mysql_get_timeout_value_ms(db) / 1000.0; // millisec -> sec
}


string DB::escape(const string &s) {
  string result;
  result.reserve(s.length());

  for (auto c: s)
    switch (c) {
    case 0:    result.append("\\0");  break;
    case '\'': result.append("\\'");  break;
    case '\"': result.append("\\\""); break;
    case '\b': result.append("\\b");  break;
    case '\n': result.append("\\n");  break;
    case '\r': result.append("\\r");  break;
    case '\t': result.append("\\t");  break;
    case 26:   result.append("\\Z");  break;
    case '\\': result.append("\\\\"); break;
    default:   result.push_back(c);   break;
    }

  return result;
}


string DB::toHex(const string &s) {
  SmartPointer<char>::Array to = new char[s.length() * 2 + 1];

  unsigned len = mysql_hex_string(to.get(), s.data(), s.length());

  return string(to.get(), len);
}


string DB::formatBool(bool      value) {return value ? "true" : "false";}
string DB::format(double        value) {return String(value);}
string DB::format(int32_t       value) {return String(value);}
string DB::format(uint32_t      value) {return String(value);}
string DB::format(const string &value) {return "'" + escape(value) + "'";}


string DB::format(const string &s, const JSON::Value &dict) {
  auto cb = [&] (const string &id, const string &spec) -> string {
    auto value = dict.select(id, 0);
    if (value.isNull()) return formatNull();
    if (spec == "s")    return format(value->asString());
    return value->formatAs(spec);
  };

  return String(s).format(cb);
}


void DB::libraryInit(int argc, char *argv[], char *groups[]) {
  if (mysql_library_init(argc, argv, groups)) THROW("Failed to init MariaDB");
}


void DB::libraryEnd() {mysql_library_end();}
const char *DB::getClientInfo() {return mysql_get_client_info();}


void DB::threadInit() {
  if (mysql_thread_init()) THROW("Failed to init MariaDB threads");
}


void DB::threadEnd() {mysql_thread_end();}
bool DB::threadSafe() {return mysql_thread_safe();}


bool DB::closeContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  status = mysql_close_cont(db, ready);
  if (status) return false;

  db = mysql_init(0);
  connected = false;
  return true;
}


bool DB::connectContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  MYSQL *db = 0;
  status = mysql_real_connect_cont(&db, this->db, ready);
  if (status) return false;

  if (!db) RAISE_DB_ERROR("Failed to connect");
  connected = true;

  return true;
}


bool DB::resetConnectionContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_reset_connection_cont(&ret, db, ready);
  if (status) return false;

  if (ret) RAISE_DB_ERROR("Failed to reset DB connection");

  return true;
}


bool DB::changeUserContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  my_bool ret = false;
  status = mysql_change_user_cont(&ret, db, ready);
  if (status) return false;

  if (ret) RAISE_DB_ERROR("Failed to change DB user");

  return true;
}


bool DB::pingContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_ping_cont(&ret, db, ready);
  if (status) return false;

  connected = !ret;

  return true;
}


bool DB::useContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_select_db_cont(&ret, this->db, ready);
  if (status) return false;

  if (ret) RAISE_DB_ERROR("Failed to select DB");

  return true;
}


bool DB::prepareContinue(unsigned ready) {
  int ret = 0;
  status = mysql_stmt_prepare_cont(&ret, stmt, ready);
  if (status) return false;
  if (ret) RAISE_DB_ERROR("Prepare failed");

  return startExecute();
}


bool DB::executeContinue(unsigned ready) {
  int ret = 0;
  status = mysql_stmt_execute_cont(&ret, stmt, ready);
  if (status) return false;
  if (ret) RAISE_DB_ERROR("Execute failed");

  return true;
}


bool DB::storeResultContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_stmt_store_result_cont(&ret, stmt, ready);
  if (status) return false;
  if (ret) RAISE_DB_ERROR("Failed to store result");

  return true;
}


bool DB::nextResultContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_stmt_next_result_cont(&ret, stmt, ready);
  if (status) return false;
  if (0 < ret) RAISE_DB_ERROR("Failed to get next result");

  return true;
}


bool DB::freeResultContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  my_bool ret = 0;
  status = mysql_stmt_free_result_cont(&ret, stmt, ready);

  return !status;
}


bool DB::fetchRowContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  status = mysql_stmt_fetch_cont(&fetchRet, stmt, ready);
  if (status) return false;

  processFetch();

  return true;
}
