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

#include "BStream.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <openssl/bio.h>
#include <openssl/opensslv.h>

#include <cstring>

using namespace cb;
using namespace std;


#define BSTREAM_DEBUG() LOG_DEBUG(5, CBANG_FUNC << "()");
#define BIO_BSTREAM(bio) ((BStream *)BIO_get_data(bio))


namespace {
  const char *cmd2str(int cmd) {
    switch (cmd) {
    case BIO_CTRL_RESET:                    return "RESET";
    case BIO_CTRL_EOF:                      return "EOF";
    case BIO_CTRL_INFO:                     return "INFO";
    case BIO_CTRL_SET:                      return "SET";
    case BIO_CTRL_GET:                      return "GET";
    case BIO_CTRL_PUSH:                     return "PUSH";
    case BIO_CTRL_POP:                      return "POP";
    case BIO_CTRL_GET_CLOSE:                return "GET_CLOSE";
    case BIO_CTRL_SET_CLOSE:                return "SET_CLOSE";
    case BIO_CTRL_PENDING:                  return "PENDING";
    case BIO_CTRL_FLUSH:                    return "FLUSH";
    case BIO_CTRL_DUP:                      return "DUP";
    case BIO_CTRL_WPENDING:                 return "WPENDING";
    case BIO_CTRL_SET_CALLBACK:             return "SET_CALLBACK";
    case BIO_CTRL_GET_CALLBACK:             return "GET_CALLBACK";
    case BIO_CTRL_SET_FILENAME:             return "SET_FILENAME";
    case BIO_CTRL_DGRAM_CONNECT:            return "DGRAM_CONNECT";
    case BIO_CTRL_DGRAM_SET_CONNECTED:      return "DGRAM_SET_CONNECTED";
    case BIO_CTRL_DGRAM_SET_RECV_TIMEOUT:   return "DGRAM_SET_RECV_TIMEOUT";
    case BIO_CTRL_DGRAM_GET_RECV_TIMEOUT:   return "DGRAM_GET_RECV_TIMEOUT";
    case BIO_CTRL_DGRAM_SET_SEND_TIMEOUT:   return "DGRAM_SET_SEND_TIMEOUT";
    case BIO_CTRL_DGRAM_GET_SEND_TIMEOUT:   return "DGRAM_GET_SEND_TIMEOUT";
    case BIO_CTRL_DGRAM_GET_RECV_TIMER_EXP: return "DGRAM_GET_RECV_TIMER_EXP";
    case BIO_CTRL_DGRAM_GET_SEND_TIMER_EXP: return "DGRAM_GET_SEND_TIMER_EXP";
    case BIO_CTRL_DGRAM_MTU_DISCOVER:       return "DGRAM_MTU_DISCOVER";
    case BIO_CTRL_DGRAM_QUERY_MTU:          return "DGRAM_QUERY_MTU";
    case BIO_CTRL_DGRAM_GET_MTU:            return "DGRAM_GET_MTU";
    case BIO_CTRL_DGRAM_SET_MTU:            return "DGRAM_SET_MTU";
    case BIO_CTRL_DGRAM_MTU_EXCEEDED:       return "DGRAM_MTU_EXCEEDED";
    case BIO_CTRL_DGRAM_GET_PEER:           return "DGRAM_GET_PEER";
    case BIO_CTRL_DGRAM_SET_PEER:           return "DGRAM_SET_PEER";
    case BIO_CTRL_DGRAM_SET_NEXT_TIMEOUT:   return "DGRAM_SET_NEXT_TIMEOUT";
    default:                                return "UNKNOWN";
    }
  }
}


static int bstream_write(BIO *bio, const char *buf, int length) {
  BSTREAM_DEBUG();
  return BIO_BSTREAM(bio)->write(buf, length);
}


static int bstream_read(BIO *bio, char *buf, int length) {
  BSTREAM_DEBUG();
  return BIO_BSTREAM(bio)->read(buf, length);
}


static int bstream_puts(BIO *bio, const char *buf) {
  BSTREAM_DEBUG();
  return BIO_BSTREAM(bio)->puts(buf);
}


static int bstream_gets(BIO *bio, char *buf, int length) {
  BSTREAM_DEBUG();
  return BIO_BSTREAM(bio)->gets(buf, length);
}


static long bstream_ctrl(BIO *bio, int cmd, long sub, void *arg) {
  BSTREAM_DEBUG();
  return BIO_BSTREAM(bio)->ctrl(cmd, sub, arg);
}


static int bstream_create(BIO *bio) {
  BSTREAM_DEBUG();
  BIO_set_init(bio, 1);
  return 1;
}


static int bstream_destroy(BIO *bio) {
  BSTREAM_DEBUG();
  return BIO_BSTREAM(bio)->destroy();
}


BStream::BStream() {
  static BIO_METHOD *method = 0;

  if (!method) {
    method = BIO_meth_new(BIO_TYPE_FD, "iostream");
    if (!method) THROW("Failed to create BIO_METHOD object");

    BIO_meth_set_write  (method, bstream_write);
    BIO_meth_set_read   (method, bstream_read);
    BIO_meth_set_puts   (method, bstream_puts);
    BIO_meth_set_gets   (method, bstream_gets);
    BIO_meth_set_ctrl   (method, bstream_ctrl);
    BIO_meth_set_create (method, bstream_create);
    BIO_meth_set_destroy(method, bstream_destroy);
  }

  bio = BIO_new(method);
  if (!bio) THROW("Failed to create BIO object");

  BIO_set_data(bio, this);
}


BStream::~BStream() {if (bio) BIO_free(bio);}


void BStream::setFlags  (int flags) {BIO_set_flags  (bio, flags);}
void BStream::clearFlags(int flags) {BIO_clear_flags(bio, flags);}


int BStream::write(const char *buf, int length) {
  return -2; // Not implemented
}


int BStream::read(char *buf, int length) {
  return -2; // Not implemented
}


int BStream::puts(const char *buf) {
  return write(buf, strlen(buf));
}


int BStream::gets(char *buf, int length) {
  LOG_DEBUG(3, "BStream::gets(" << length << ")");
  return -2; // Not implemented
}


long BStream::ctrl(int cmd, long sub, void *arg) {
  const char *cmdStr = cmd2str(cmd);
  LOG_DEBUG(5, "BStream::ctrl(" << cmdStr << '=' << cmd << ", " << sub << ")");
  return 1; // Success
}


int BStream::destroy() {return 1;}
