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

#include "Request.h"
#include "BufferDevice.h"
#include "Connection.h"
#include "Event.h"
#include "HTTP.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>
#include <cbang/openssl/SSL.h>
#include <cbang/log/Logger.h>
#include <cbang/http/Cookie.h>
#include <cbang/json/JSON.h>
#include <cbang/time/Time.h>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
namespace io = boost::iostreams;

using namespace cb::Event;
using namespace cb;
using namespace std;


namespace {
  struct FilteringOStreamWithRef : public io::filtering_ostream {
    SmartPointer<ostream> ref;
    virtual ~FilteringOStreamWithRef() {reset();}
  };


  SmartPointer<ostream> compressBufferStream
  (cb::Event::Buffer buffer, Request::compression_t compression) {
    SmartPointer<ostream> target = new BufferStream<>(buffer);
    SmartPointer<FilteringOStreamWithRef> out = new FilteringOStreamWithRef;

    switch (compression) {
    case Request::COMPRESS_ZLIB:  out->push(io::zlib_compressor()); break;
    case Request::COMPRESS_GZIP:  out->push(io::gzip_compressor()); break;
    case Request::COMPRESS_BZIP2: out->push(io::bzip2_compressor()); break;
    default: return target;
    }

    out->ref = target;
    out->push(*target);

    return out;
  }


  const char *getContentEncoding(Request::compression_t compression) {
    switch (compression) {
    case Request::COMPRESS_ZLIB:  return "zlib";
    case Request::COMPRESS_GZIP:  return "gzip";
    case Request::COMPRESS_BZIP2: return "bzip2";
    default: return 0;
    }
  }


  struct JSONWriter :
    cb::Event::Buffer, SmartPointer<ostream>, public JSON::Writer {
    SmartPointer<Request> req;
    bool closed = false;

    JSONWriter(const SmartPointer<Request> &req, unsigned indent, bool compact,
               Request::compression_t compression) :
      SmartPointer<ostream>(compressBufferStream(*this, compression)),
      JSON::Writer(*SmartPointer<ostream>::get(), indent, compact), req(req) {
      req->outSetContentEncoding(compression);
    }

    ~JSONWriter() {TRY_CATCH_ERROR(close(););}

    ostream &getStream() {return *SmartPointer<ostream>::get();}
    unsigned getID() {return req->getID();}

    // From JSON::NullSink
    void close() {
      if (closed) return;
      closed = true;
      JSON::Writer::close();
      SmartPointer<ostream>::get()->flush();
      if (!getLength()) req->outRemove("Content-Type");
      send(*this);
    }

    virtual void send(cb::Event::Buffer &buffer) {req->send(buffer);}
  };
}


#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX << "REQ" << getID() << ':'


Request::Request(RequestMethod method, const URI &uri, const Version &version) :
  method(method), originalURI(uri), uri(uri), version(version) {
  LOG_DEBUG(4, "created");
}


Request::~Request() {LOG_DEBUG(4, "destroyed");}


uint64_t Request::getID() const {
  return connection.isSet() ? connection->getID() : 0;
}


string Request::getResponseLine() const {
  return SSTR("HTTP/" << version << ' ' << (int)responseCode << ' '
              << (responseCodeLine.empty() ?
                  responseCode.getDescription() : responseCodeLine));
}


string Request::getRequestLine() const {
  return SSTR(method << ' ' << uri << " HTTP/" << version);
}


bool Request::isConnected() const {
  return hasConnection() && getConnection().isConnected();
}


string Request::getSessionID(const string &cookie, const string &header) const {
  return inHas(header) ? inGet(header) : findCookie(cookie);
}


const string &Request::getUser() const {
  return (session.isNull() || !session->hasUser()) ? user : session->getUser();
}


void Request::setUser(const string &user) {
  this->user = user;
  if (!session.isNull()) session->setUser(user);
}


bool Request::isSecure() const {
  return connection.isSet() && connection->hasSSL();
}


SSL Request::getSSL() const {return connection->getSSL();}
void Request::resetOutput() {getOutputBuffer().clear();}


JSON::Value &Request::parseJSONArgs() {
  Headers &hdrs = getInputHeaders();

  if (hdrs.hasContentType() &&
      String::startsWith(hdrs.getContentType(), "application/json")) {

    Buffer buf = getInputBuffer();
    if (buf.getLength()) {
      BufferStream<> stream(buf);
      JSON::Reader reader(stream);

      // Find start of dict & parse keys into request args
      if (reader.next() == '{') {
        JSON::ValuePtr argsPtr = JSON::ValuePtr::Phony(&args);
        JSON::Builder builder(argsPtr);
        reader.parseDict(builder);
      }
    }
  }

  return args;
}


JSON::Value &Request::parseQueryArgs() {
  const URI &uri = getURI();
  for (URI::const_iterator it = uri.begin(); it != uri.end(); it++)
    insertArg(it->first, it->second);
  return args;
}


JSON::Value &Request::parseArgs() {
  parseJSONArgs();
  parseQueryArgs();
  return args;
}


const IPAddress &Request::getClientIP() const {return connection->getPeer();}


bool Request::inHas(const string &name) const {
  return getInputHeaders().has(name);
}


string Request::inFind(const string &name) const {
  return getInputHeaders().find(name);
}


string Request::inGet(const string &name) const {
  return getInputHeaders().get(name);
}


void Request::inSet(const string &name, const string &value) {
  getInputHeaders().insert(name, value);
}


void Request::inRemove(const string &name) {getInputHeaders().remove(name);}


bool Request::outHas(const string &name) const {
  return getOutputHeaders().has(name);
}


string Request::outFind(const string &name) const {
  return getOutputHeaders().find(name);
}


string Request::outGet(const string &name) const {
  return getOutputHeaders().get(name);
}


void Request::outSet(const string &name, const string &value) {
  getOutputHeaders().insert(name, value);
}


void Request::outRemove(const string &name) {
  getOutputHeaders().remove(name);
}


void Request::setPersistent(bool x) {
  if (version < Version(1, 1)) {
    if (x) outSet("Connection", "Keep-Alive");
    else outRemove("Connection");

  } else if (x) outRemove("Connection");
  else outSet("Connection", "close");
}


void Request::setCache(uint32_t age) {
  const char *format = "%a, %d %b %Y %H:%M:%S GMT";
  string now = Time(format).toString();

  outSet("Date", now);

  if (age) {
    outSet("Cache-Control", "max-age=" + String(age));
    outSet("Expires", Time(Time::now() + age, format).toString());

  } else {
    outSet("Cache-Control", "max-age=0, no-cache, no-store");
    outSet("Expires", now);
  }
}


bool Request::hasContentType() const {
  return getOutputHeaders().hasContentType();
}


string Request::getContentType() const {
  return getOutputHeaders().getContentType();
}


void Request::setContentType(const string &contentType) {
  getOutputHeaders().setContentType(contentType);
}


void Request::guessContentType() {
  getOutputHeaders().guessContentType(uri.getExtension());
}


void Request::outSetContentEncoding(compression_t compression) {
  switch (compression) {
  case COMPRESS_ZLIB:
  case COMPRESS_GZIP:
  case COMPRESS_BZIP2:
    outSet("Content-Encoding", getContentEncoding(compression));
    break;
  default: break;
  }
}


Request::compression_t Request::getRequestedCompression() const {
  if (!inHas("Accept-Encoding")) return COMPRESS_NONE;

  vector<string> accept;
  String::tokenize(inGet("Accept-Encoding"), accept, ", \t");

  double maxQ = 0;
  double otherQ = 0;
  set<string> named;
  compression_t compression = COMPRESS_NONE;

  for (unsigned i = 0; i < accept.size(); i++) {
    double q = 1;
    string name = String::toLower(accept[i]);

    // Check for quality value
    size_t pos = name.find_first_of(';');
    if (pos != string::npos) {
      string arg = name.substr(pos + 1);
      name = name.substr(0, pos);

      if (2 < arg.length() && arg[0] == 'q' && arg[1] == '=') {
        q = String::parseDouble(arg.substr(2));
        if (name == "*") otherQ = q;
      }
    }

    named.insert(name);

    if (maxQ < q) {
      if (name == "identity")   compression = COMPRESS_NONE;
      else if (name == "gzip")  compression = COMPRESS_GZIP;
      else if (name == "zlib")  compression = COMPRESS_ZLIB;
      else if (name == "bzip2") compression = COMPRESS_BZIP2;
      else q = 0;
    }

    if (maxQ < q) maxQ = q;
  }

  // Currently, the only standard compression format we support is gzip, so
  // if the user specifies something like "*;q=1" and doesn't give gzip
  // an explicit quality value then we select gzip compression.
  if (maxQ < otherQ && named.find("gzip") == named.end())
    compression = COMPRESS_GZIP;

  return compression;
}


bool Request::hasCookie(const string &name) const {
  if (!inHas("Cookie")) return false;

  vector<string> cookies;
  String::tokenize(inGet("Cookie"), cookies, "; \t\n\r");

  for (unsigned i = 0; i < cookies.size(); i++)
    if (name == cookies[i].substr(0, cookies[i].find('='))) return true;

  return false;
}


string Request::findCookie(const string &name) const {
  if (inHas("Cookie")) {
    // Return only the first matching cookie
    vector<string> cookies;
    String::tokenize(inGet("Cookie"), cookies, "; \t\n\r");

    for (unsigned i = 0; i < cookies.size(); i++) {
      size_t pos = cookies[i].find('=');

      if (name == cookies[i].substr(0, pos))
        return pos == string::npos ? string() : cookies[i].substr(pos + 1);
    }
  }

  return "";
}


string Request::getCookie(const string &name) const {
  if (!hasCookie(name)) THROW("Cookie '" << name << "' not set");
  return findCookie(name);
}


void Request::setCookie(const string &name, const string &value,
                        const string &domain, const string &path,
                        uint64_t expires, uint64_t maxAge, bool httpOnly,
                        bool secure) {
  outSet("Set-Cookie", cb::HTTP::Cookie(name, value, domain, path, expires,
                                        maxAge, httpOnly, secure).toString());
}


string Request::getInput() const {return getInputBuffer().toString();}
string Request::getOutput() const {return getOutputBuffer().toString();}


SmartPointer<JSON::Value> Request::getInputJSON() const {
  Buffer buf = getInputBuffer();
  if (!buf.getLength()) return 0;
  BufferStream<> stream(buf);
  return JSON::Reader(stream).parse();
}


SmartPointer<JSON::Value> Request::getJSONMessage() const {
  Headers hdrs = getInputHeaders();

  if (hdrs.hasContentType() &&
      String::startsWith(hdrs.getContentType(), "application/json"))
    return getInputJSON();

  SmartPointer<JSON::Value> msg;
  const URI &uri = getURI();

  if (!uri.empty()) {
    msg = new JSON::Dict;

    for (URI::const_iterator it = uri.begin(); it != uri.end(); it++)
      msg->insert(it->first, it->second);
  }

  return msg;
}


SmartPointer<JSON::Writer>
Request::getJSONWriter(unsigned indent, bool compact,
                       compression_t compression) {
  resetOutput();
  setContentType("application/json");

  return new JSONWriter(this, indent, compact, compression);
}


SmartPointer<JSON::Writer> Request::getJSONWriter(compression_t compression) {
  return getJSONWriter(0, !getURI().has("pretty"), compression);
}


SmartPointer<JSON::Writer> Request::getJSONPWriter(const string &callback) {
  struct Writer : public JSONWriter {
    Writer(const SmartPointer<Request> &req, const string &callback) :
      JSONWriter(req, 0, true, COMPRESS_NONE) {
      getStream() << callback << '(';
    }

    void close() {
      JSON::Writer::close();
      getStream() << ')';
      JSONWriter::close();
    }
  };

  resetOutput();
  setContentType("application/javascript");

  return new Writer(this, callback);
}


SmartPointer<istream> Request::getInputStream() const {
  return new BufferStream<>(getInputBuffer());
}


SmartPointer<ostream> Request::getOutputStream(compression_t compression) {
  // Auto select compression type based on Accept-Encoding
  if (compression == COMPRESS_AUTO) compression = getRequestedCompression();
  outSetContentEncoding(compression);
  return compressBufferStream(getOutputBuffer(), compression);
}


void Request::sendError(HTTPStatus code) {
  if (getContentType() == "application/json") return sendJSONError(code, "");

  outSet("Content-Type", "text/html");
  outSet("Connection", "close");

  if (!code) code = HTTP_INTERNAL_SERVER_ERROR;
  string msg = String((int)code) + " " + code.getDescription();

  send(SSTR("<html><head><title>" << msg << "</title></head><body><h1>"
            << msg << "</h1></body></html>"));

  reply(code);
}


void Request::sendError(HTTPStatus code, const string &msg) {
  if (getContentType() == "application/json") return sendJSONError(code, msg);

  resetOutput();
  setContentType("text/plain");
  send(msg);
  sendError(code);
}


void Request::sendError(const Exception &e) {
  HTTPStatus code = (HTTPStatus::enum_t)e.getTopCode();
  if (!code) code = HTTP_INTERNAL_SERVER_ERROR;

  if (getContentType() == "application/json") {
    auto writer = getJSONWriter();

    writer->beginDict();
    writer->beginInsert("error");
    e.write(*writer, false);
    writer->endDict();
    writer->close();

    reply(code);

  } else sendError(code, e.getMessages());
}


void Request::sendError(const exception &e) {
  sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR, e.what());
}


void Request::sendJSONError(HTTPStatus code, const string &message) {
  auto writer = getJSONWriter();

  writer->beginDict();
  writer->insert("error", message);
  writer->endDict();
  writer->close();

  if (!code) code = HTTP_INTERNAL_SERVER_ERROR;
  reply(code);
}


void Request::send(const cb::Event::Buffer &buf) {getOutputBuffer().add(buf);}


void Request::send(const char *data, unsigned length) {
  getOutputBuffer().add(data, length);
}


void Request::send(const char *s) {getOutputBuffer().add(s);}
void Request::send(const string &s) {getOutputBuffer().add(s);}
void Request::sendFile(const string &path) {getOutputBuffer().addFile(path);}


void Request::reply(HTTPStatus code) {
  if (replying) THROW("Request already replying");
  replying = true;

  if (code) responseCode = code;
  else responseCode = HTTP_INTERNAL_SERVER_ERROR;

  // Log results
  LOG_DEBUG(5, getResponseLine() << '\n' << getOutputHeaders() << '\n');
  LOG_DEBUG(6, getOutputBuffer().hexdump() << '\n');

  write();
}


void Request::reply(const cb::Event::Buffer &buf) {reply(HTTP_OK, buf);}


void Request::reply(const char *data, unsigned length) {
  reply(HTTP_OK, data, length);
}


void Request::reply(HTTPStatus code, const char *data, unsigned length) {
  reply(code, Buffer(data, length));
}


void Request::startChunked(HTTPStatus code) {
  if (outHas("Content-Length"))
    THROW("Cannot start chunked with Content-Length set");
  if (version < Version(1, 1))
    THROW("Cannot start chunked with HTTP version " << version);
  if (!mustHaveBody()) THROW("Cannot start chunked with " << method);
  if (outputBuffer.getLength())
    THROW("Cannot start chunked data in output buffer");

  outSet("Transfer-Encoding", "chunked");
  chunked = true;
  reply(code);
}


void Request::sendChunk(const cb::Event::Buffer &buf) {
  if (!chunked) THROW("Not chunked");

  LOG_DEBUG(4, "Sending " << buf.getLength() << " byte chunk");
  bool empty = !buf.getLength(); // Must be before add() below

  Buffer out;
  out.add(String::printf("%x\r\n", buf.getLength()));
  out.add(buf);
  out.add("\r\n");

  connection->write(*this, out);

  if (empty) chunked = false; // Last chunk is empty
}


void Request::sendChunk(const char *data, unsigned length) {
  sendChunk(Buffer(data, length));
}


SmartPointer<JSON::Writer> Request::getJSONChunkWriter() {
  struct Writer : public JSONWriter {
    Writer(const SmartPointer<Request> &req) :
      JSONWriter(req, 0, true, COMPRESS_NONE) {}

    // NOTE, destructor must be duplicated so virtual function call works
    ~Writer() {TRY_CATCH_ERROR(close(););}

    // From JSONWriter
    void send(Buffer &buffer) {req->sendChunk(buffer);}
  };

  return new Writer(this);
}


void Request::endChunked() {sendChunk(Buffer(""));}


void Request::redirect(const URI &uri, HTTPStatus code) {
  outSet("Location", uri);
  outSet("Content-Length", "0");
  reply(code, "", 0);
}


void Request::cancel() {connection->cancelRequest(*this);}


void Request::onRequest() {
  LOG_INFO(1, "< " << getRequestLine());
  LOG_DEBUG(5, inputHeaders << '\n');
  LOG_DEBUG(6, inputBuffer.hexdump() << '\n');

  if (connection.isSet()) {
    bytesRead = connection->getHeaderSize() + connection->getBodySize();

    if (connection->getHTTP().isSet())
      connection->getHTTP()->handleRequest(*this);
  }
}


bool Request::mustHaveBody() const {
  return responseCode != HTTP_NO_CONTENT && responseCode != HTTP_NOT_MODIFIED &&
    (200 <= responseCode || responseCode < 100) && method != HTTP_HEAD;
}


bool Request::mayHaveBody() const {
  return method == HTTP_POST || method == HTTP_PUT || method == HTTP_PATCH;
}


bool Request::needsClose() const {
  return inputHeaders.needsClose() || outputHeaders.needsClose();
}


Version Request::parseHTTPVersion(const string &s) {
  if (!String::startsWith(s, "HTTP/"))
    THROW("Expected 'HTTP/' got '" << s << "'");
  return Version(s.substr(5));
}


void Request::parseResponseLine(const string &line) {
  vector<string> parts;
  String::tokenize(line, parts, " ", false, 3);

  if (parts.size() < 2) THROW("Invalid HTTP response line: " << line);
  version = parseHTTPVersion(parts[0]);

  responseCode = HTTPStatus::parse(parts[1]);
  if (!responseCode) THROW("Bad response code " << parts[1]);

  if (parts.size() == 3) responseCodeLine = parts[2];
}


void Request::write() {
  Buffer out;

  writeHeaders(out);
  if (outputBuffer.getLength()) out.add(outputBuffer);

  bytesWritten += out.getLength();
  connection->write(*this, out);
}


void Request::writeResponse(cb::Event::Buffer &buf) {
  buf.add(getResponseLine() + "\r\n");

  if (version.getMajor() == 1) {
    if (1 <= version.getMinor() && !outHas("Date"))
      outSet("Date", Time("%a, %d %b %Y %H:%M:%S GMT"));

    // if the protocol is 1.0 and connection was keep-alive add keep-alive
    bool keepAlive = inputHeaders.connectionKeepAlive();
    if (!version.getMinor() && keepAlive) outSet("Connection", "keep-alive");

    if ((0 < version.getMinor() || keepAlive) && mustHaveBody() &&
        !outHas("Transfer-Encoding") && !outHas("Content-Length"))
      outSet("Content-Length", String(outputBuffer.getLength()));
  }

  // Add Content-Type
  if (mustHaveBody() && !hasContentType()) {
    guessContentType();

    if (!hasContentType()) {
      auto &type = connection->getDefaultContentType();
      if (!type.empty()) outSet("Content-Type", type);
    }
  }

  // If request asked for close, send close
  if (inputHeaders.needsClose()) outSet("Connection", "close");
}


void Request::writeRequest(cb::Event::Buffer &buf) {
  // Generate request line
  buf.add(getRequestLine() + "\r\n");

  // Add the content length on a post or put request if missing
  if ((method == HTTP_POST || method == HTTP_PUT) && !outHas("Content-Length"))
    outSet("Content-Length", String(outputBuffer.getLength()));
}


void Request::writeHeaders(cb::Event::Buffer &buf) {
  if (connection->isIncoming()) writeResponse(buf);
  else writeRequest(buf);

  for (auto it = outputHeaders.begin(); it != outputHeaders.end(); it++) {
    const string &key = it->first;
    const string &value = it->second;
    if (!value.empty())
      buf.add(String::printf("%s: %s\r\n", key.c_str(), value.c_str()));
  }

  buf.add("\r\n");
}
