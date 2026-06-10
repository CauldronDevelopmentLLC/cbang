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

#include "FakeMariaDB.h"

#include <mysql/mysql.h>

#include <sys/eventfd.h>
#include <unistd.h>

#include <deque>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>

using namespace std;
using namespace FakeDB;


namespace {
  // Field metadata, returned by mysql_stmt_result_metadata().  `res` is FIRST
  // so (Meta *)res-pointer == this.
  struct Meta {
    MYSQL_RES res;
    vector<string>      names;
    vector<MYSQL_FIELD> fields;
    Meta() {memset(&res, 0, sizeof(res));}
  };


  // A connection.  Opaque to cbang; the statement handle is the same pointer
  // (cbang never dereferences either), so one object backs both.
  struct Conn {
    string   pendingSQL;
    Response cur;
    bool     haveCur = false;
    size_t   setIdx  = 0;
    size_t   pos     = 0;            // next row to fetch in the current set
    const vector<Cell> *row = 0;     // row last returned by fetch
    MYSQL_BIND *rbind = 0;           // bound result columns (mysql_stmt_bind_result)
    unsigned errnoVal = 0;
    string   error;
    string   sqlstate = "00000";
  };


  deque<Response> g_pending;   // queued query outcomes, FIFO
  vector<string>  g_queries;   // captured SQL, in order
  vector<vector<string>> g_binds; // captured bound params, per query
  int             g_fd = -1;   // always-readable fd backing every NB wait

  const int PENDING = MYSQL_WAIT_READ;

  int readyFD() {
    if (g_fd < 0) g_fd = eventfd(1, EFD_NONBLOCK | EFD_CLOEXEC);
    return g_fd;
  }

  Conn *C(MYSQL *m)      {return reinterpret_cast<Conn *>(m);}
  Conn *S(MYSQL_STMT *s) {return reinterpret_cast<Conn *>(s);}
  Meta *M(MYSQL_RES *r)  {return reinterpret_cast<Meta *>(r);}

  bool hasSet(Conn *c) {return c->haveCur && c->setIdx < c->cur.results.size();}
  const Result *curSet(Conn *c) {return hasSet(c) ? &c->cur.results[c->setIdx] : 0;}
}


// --- FakeDB controller -----------------------------------------------------
void FakeDB::reset() {g_pending.clear(); g_queries.clear(); g_binds.clear();}
void FakeDB::push(const Response &r) {g_pending.push_back(r);}
const vector<string> &FakeDB::queries() {return g_queries;}
const vector<vector<string>> &FakeDB::binds() {return g_binds;}


// --- connection / library --------------------------------------------------
MYSQL *mysql_init(MYSQL *) {return reinterpret_cast<MYSQL *>(new Conn);}
void   mysql_close(MYSQL *m) {delete C(m);}

int  mysql_options(MYSQL *, enum mysql_option, const void *) {return 0;}
int  mysql_server_init(int, char **, char **) {return 0;}
void mysql_server_end(void) {}
const char  *mysql_get_client_info(void) {return "fake-mariadb";}
my_bool      mysql_thread_init(void) {return 0;}
void         mysql_thread_end(void) {}
unsigned int mysql_thread_safe(void) {return 1;}

my_socket    mysql_get_socket(MYSQL *) {return readyFD();}
unsigned int mysql_get_timeout_value_ms(const MYSQL *) {return 0;}

const char  *mysql_error(MYSQL *m)        {return C(m)->error.c_str();}
unsigned int mysql_errno(MYSQL *m)        {return C(m)->errnoVal;}
const char  *mysql_sqlstate(MYSQL *m)     {return C(m)->sqlstate.c_str();}
const char  *mysql_info(MYSQL *)          {return "";}
unsigned int mysql_warning_count(MYSQL *) {return 0;}

MYSQL *mysql_real_connect(MYSQL *m, const char *, const char *, const char *,
  const char *, unsigned int, const char *, unsigned long) {return m;}
int mysql_real_connect_start(MYSQL **, MYSQL *, const char *, const char *,
  const char *, const char *, unsigned int, const char *, unsigned long) {
  return PENDING;
}
int mysql_real_connect_cont(MYSQL **ret, MYSQL *m, int) {*ret = m; return 0;}

int mysql_ping(MYSQL *) {return 0;}
int mysql_ping_start(int *, MYSQL *) {return PENDING;}
int mysql_ping_cont(int *ret, MYSQL *, int) {*ret = 0; return 0;}

my_bool mysql_change_user(MYSQL *, const char *, const char *, const char *) {
  return 0;
}
int mysql_change_user_start(my_bool *, MYSQL *, const char *, const char *,
  const char *) {return PENDING;}
int mysql_change_user_cont(my_bool *ret, MYSQL *, int) {*ret = 0; return 0;}

int mysql_select_db(MYSQL *, const char *) {return 0;}
int mysql_select_db_start(int *, MYSQL *, const char *) {return PENDING;}
int mysql_select_db_cont(int *ret, MYSQL *, int) {*ret = 0; return 0;}

int mysql_reset_connection(MYSQL *) {return 0;}
int mysql_reset_connection_start(int *, MYSQL *) {return PENDING;}
int mysql_reset_connection_cont(int *ret, MYSQL *, int) {*ret = 0; return 0;}

int mysql_close_start(MYSQL *) {return PENDING;}
int mysql_close_cont(MYSQL *, int) {return 0;}

unsigned long mysql_hex_string(char *to, const char *from, unsigned long len) {
  static const char hex[] = "0123456789ABCDEF";
  char *p = to;
  for (unsigned long i = 0; i < len; i++) {
    *p++ = hex[(unsigned char)from[i] >> 4];
    *p++ = hex[(unsigned char)from[i] & 0xf];
  }
  *p = 0;
  return len * 2;
}


// --- result metadata (MYSQL_RES) -------------------------------------------
void mysql_free_result(MYSQL_RES *r) {delete M(r);}
unsigned int mysql_num_fields(MYSQL_RES *r) {return M(r)->fields.size();}
MYSQL_FIELD *mysql_fetch_fields(MYSQL_RES *r) {return M(r)->fields.data();}


// --- prepared statements ---------------------------------------------------
// The statement handle is the connection pointer; mysql_close frees it, so
// mysql_stmt_close is a no-op (avoids a double free).
MYSQL_STMT *mysql_stmt_init(MYSQL *m) {return reinterpret_cast<MYSQL_STMT *>(m);}
my_bool     mysql_stmt_close(MYSQL_STMT *) {return 0;}

int mysql_stmt_prepare_start(int *, MYSQL_STMT *s, const char *q,
  unsigned long len) {
  S(s)->pendingSQL.assign(q, len);
  return PENDING;
}
int mysql_stmt_prepare_cont(int *ret, MYSQL_STMT *s, int) {
  Conn *c = S(s);
  g_queries.push_back(c->pendingSQL);
  g_binds.push_back({}); // filled by mysql_stmt_bind_param, if called

  if (g_pending.empty()) c->cur = Response();
  else {c->cur = g_pending.front(); g_pending.pop_front();}

  c->haveCur = true;
  c->setIdx  = 0;
  c->pos     = 0;
  c->row     = 0;
  c->rbind   = 0;
  c->errnoVal = 0;
  c->error.clear();
  c->sqlstate = "00000";

  *ret = 0; // a failure (if any) surfaces at execute, as in a stored proc
  return 0;
}

unsigned long mysql_stmt_param_count(MYSQL_STMT *s) {
  // Count ``?`` placeholders outside single-quoted literals
  unsigned long n = 0;
  bool quoted = false;
  for (char c: S(s)->pendingSQL) {
    if (c == '\'') quoted = !quoted;
    else if (c == '?' && !quoted) n++;
  }
  return n;
}

my_bool mysql_stmt_bind_param(MYSQL_STMT *s, MYSQL_BIND *b) {
  unsigned long n = mysql_stmt_param_count(s);
  for (unsigned long i = 0; i < n; i++)
    g_binds.back().push_back(
      b[i].buffer_type == MYSQL_TYPE_NULL ? "\\N" : // mysqldump NULL marker
      string((const char *)b[i].buffer, b[i].buffer_length));
  return 0;
}

// Execute demands a WRITE wait, as the real client does when a large bound
// parameter overflows the socket buffer.  An event re-armed with stale READ
// flags never completes (the bug this guards against spins the driver loop).
int mysql_stmt_execute_start(int *, MYSQL_STMT *) {return MYSQL_WAIT_WRITE;}
int mysql_stmt_execute_cont(int *ret, MYSQL_STMT *s, int ready) {
  if (!(ready & MYSQL_WAIT_WRITE)) return MYSQL_WAIT_WRITE; // still pending
  Conn *c = S(s);
  if (c->cur.errnoVal) {
    c->errnoVal = c->cur.errnoVal;
    c->error    = c->cur.error;
    c->sqlstate = c->cur.sqlstate;
    *ret = 1;
  } else *ret = 0;
  return 0;
}

MYSQL_RES *mysql_stmt_result_metadata(MYSQL_STMT *s) {
  const Result *set = curSet(S(s));
  if (!set || set->cols.empty()) return 0; // no result set

  auto m = new Meta;
  m->names .resize(set->cols.size());
  m->fields.resize(set->cols.size());
  for (size_t i = 0; i < set->cols.size(); i++) {
    m->names[i] = set->cols[i].name;
    MYSQL_FIELD f;
    memset(&f, 0, sizeof(f));
    f.name        = const_cast<char *>(m->names[i].c_str());
    f.name_length = m->names[i].size();
    f.type        = (enum_field_types)set->cols[i].type;
    m->fields[i]  = f;
  }

  return &m->res;
}

my_bool mysql_stmt_bind_result(MYSQL_STMT *s, MYSQL_BIND *b) {
  S(s)->rbind = b;
  return 0;
}

int mysql_stmt_store_result_start(int *, MYSQL_STMT *) {return PENDING;}
int mysql_stmt_store_result_cont(int *ret, MYSQL_STMT *s, int) {
  S(s)->pos = 0;
  *ret = 0;
  return 0;
}

int mysql_stmt_fetch_start(int *, MYSQL_STMT *) {return PENDING;}
int mysql_stmt_fetch_cont(int *ret, MYSQL_STMT *s, int) {
  Conn *c = S(s);
  const Result *set = curSet(c);

  if (!set || c->pos >= set->rows.size()) {c->row = 0; *ret = MYSQL_NO_DATA; return 0;}

  c->row = &set->rows[c->pos++];

  // Report null + length per column; the data is pulled by fetch_column.
  for (size_t i = 0; c->rbind && i < c->row->size(); i++) {
    const Cell &cell = (*c->row)[i];
    if (c->rbind[i].is_null) *c->rbind[i].is_null = cell.null;
    if (c->rbind[i].length)  *c->rbind[i].length  = cell.null ? 0 : cell.data.size();
  }

  *ret = 0;
  return 0;
}

int mysql_stmt_fetch_column(MYSQL_STMT *s, MYSQL_BIND *b, unsigned int col,
  unsigned long offset) {
  Conn *c = S(s);
  if (!c->row || col >= c->row->size()) return 0;

  const Cell &cell = (*c->row)[col];
  if (b->length)  *b->length  = cell.data.size();
  if (b->is_null) *b->is_null = cell.null;

  if (!cell.null && b->buffer && b->buffer_length && offset < cell.data.size()) {
    unsigned long n = cell.data.size() - offset;
    if (n > b->buffer_length) n = b->buffer_length;
    memcpy(b->buffer, cell.data.data() + offset, n);
  }

  return 0;
}

int mysql_stmt_free_result_start(my_bool *, MYSQL_STMT *) {return PENDING;}
int mysql_stmt_free_result_cont(my_bool *ret, MYSQL_STMT *, int) {
  *ret = 0;
  return 0;
}

my_bool mysql_stmt_more_results(MYSQL_STMT *s) {
  Conn *c = S(s);
  return c->haveCur && c->setIdx + 1 < c->cur.results.size();
}
int mysql_stmt_next_result_start(int *, MYSQL_STMT *) {return PENDING;}
int mysql_stmt_next_result_cont(int *ret, MYSQL_STMT *s, int) {
  Conn *c = S(s);
  if (c->setIdx + 1 < c->cur.results.size()) {
    c->setIdx++;
    c->pos = 0;
    c->row = 0;
    *ret = 0;
  } else *ret = -1;
  return 0;
}

unsigned long long mysql_stmt_num_rows(MYSQL_STMT *s) {
  const Result *set = curSet(S(s));
  return set ? set->rows.size() : 0;
}
unsigned long long mysql_stmt_affected_rows(MYSQL_STMT *) {return 0;}
unsigned int mysql_stmt_errno(MYSQL_STMT *s)    {return S(s)->errnoVal;}
const char  *mysql_stmt_error(MYSQL_STMT *s)    {return S(s)->error.c_str();}
const char  *mysql_stmt_sqlstate(MYSQL_STMT *s) {return S(s)->sqlstate.c_str();}
