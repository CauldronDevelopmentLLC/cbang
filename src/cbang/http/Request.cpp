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

#include "Request.h"
#include "Conn.h"
#include "Cookie.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>
#include <cbang/event/Event.h>
#include <cbang/event/BufferStream.h>
#include <cbang/openssl/SSL.h>
#include <cbang/log/Logger.h>
#include <cbang/json/JSON.h>
#include <cbang/time/Time.h>
#include <cbang/util/Regex.h>
#include <cbang/comp/CompressionFilter.h>
#include <cbang/boost/IOStreams.h>

using namespace cb::HTTP;
using namespace cb;
using namespace std;


namespace {
  const char *getContentEncoding(Compression compression) {
    switch (compression) {
    case Compression::COMPRESSION_ZLIB:  return "zlib";
    case Compression::COMPRESSION_GZIP:  return "gzip";
    case Compression::COMPRESSION_BZIP2: return "bzip2";
    case Compression::COMPRESSION_LZ4:   return "lz4";
    default: return 0;
    }
  }
}


#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX (isIncoming() ? "REQ" : "OUT") << getID() << ':'


Request::Request(const RequestParams &params) :
  inputHeaders(params.hdrs), connection(params.connection),
  method(params.method), uri(params.uri), version(params.version),
  args(new JSON::Dict) {}


Request::~Request() {}


uint64_t Request::getID() const {
  return connection.isSet() ? connection->getID() : 0;
}


Headers &Request::getInputHeaders() {
  if (inputHeaders.isNull()) inputHeaders = new Headers;
  return *inputHeaders;
}


Headers &Request::getOutputHeaders() {
  if (outputHeaders.isNull()) outputHeaders = new Headers;
  return *outputHeaders;
}


bool Request::isOk() const {
  return 200 <= responseCode && responseCode < 300 && !connError;
}


string Request::getResponseLine() const {
  return SSTR("HTTP/" << version << ' ' << (int)responseCode << ' '
              << (responseCodeLine.empty() ?
                  responseCode.getDescription() : responseCodeLine));
}


string Request::getRequestLine() const {
  string path;
  if (method == HTTP_CONNECT)
    path = uri.getHost() + ":" + String(uri.getPort());
  else if (version == Version(1, 0)) path = uri.toString();
  else path = uri.getEscapedPathAndQuery();

  return SSTR(method << ' ' << path << " HTTP/" << version);
}


bool Request::logResponseErrors() const {
  if (connError) LOG_ERROR("Failed response: " << connError);
  else if (!isOk()) LOG_ERROR(responseCode << ": " << getInput());
  else return false;

  return true;
}


bool Request::isConnected() const {
  return hasConnection() && connection->isConnected();
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


bool Request::isIncoming() const {
  return !hasConnection() || connection->isIncoming();
}


bool Request::isSecure() const {
  return connection.isSet() && connection->getSSL().isSet();
}


SSL Request::getSSL() const {return *connection->getSSL();}


void Request::parseJSONArgs() {
  switch (method) {
  case HTTP_POST: case HTTP_PUT: case HTTP_DELETE: case HTTP_PATCH:
    getJSONMessage();
    break;

  default: break;
  }

  // Message may have been set with setJSONMessage()
  if (msg.isSet() && msg->isDict()) args->merge(*msg);
}


void Request::parseQueryArgs() {
  for (auto p: uri) args->insert(p.first, p.second);
}


const JSON::ValuePtr &Request::parseArgs() {
  parseJSONArgs();
  parseQueryArgs();
  return args;
}


SockAddr Request::getClientAddr() const {
  return connection.isSet() ? connection->getPeerAddr() : SockAddr();
}


bool Request::inHas(const string &name) const {
  return inputHeaders.isSet() && inputHeaders->has(name);
}


string Request::inFind(const string &name) const {
  return inputHeaders->find(name);
}


string Request::inGet(const string &name) const {
  return getInputHeaders().get(name);
}


void Request::inSet(const string &name, const string &value) {
  getInputHeaders().insert(name, value);
}


void Request::inRemove(const string &name) {
  if (inputHeaders.isSet()) inputHeaders->remove(name);
}


bool Request::outHas(const string &name) const {
  return outputHeaders.isSet() && outputHeaders->has(name);
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
  if (outputHeaders.isSet()) outputHeaders->remove(name);
}


bool Request::isPersistent() const {
  bool keepAlive = getInputHeaders().connectionKeepAlive();
  return (Version(1, 1) <= version || keepAlive) && !needsClose();
}


void Request::setPersistent(bool x) {
  if (version < Version(1, 1)) {
    if (x) outSet("Connection", "Keep-Alive");
    else outRemove("Connection");

  } else if (x) outRemove("Connection");
  else outSet("Connection", "close");
}


void Request::setCache(uint32_t age) {
  string now = Time().toString(Time::httpFormat);

  outSet("Date", now);

  if (age) {
    outSet("Cache-Control", "max-age=" + String(age));
    outSet("Expires", Time(Time::now() + age).toString(Time::httpFormat));

  } else {
    outSet("Cache-Control", "max-age=0, no-cache, no-store");
    outSet("Expires", now);
  }
}


bool Request::hasContentType() const {
  return outputHeaders.isSet() && outputHeaders->hasContentType();
}


string Request::getContentType() const {
  return getOutputHeaders().getContentType();
}


bool Request::isJSONContentType() const {
  return outputHeaders.isSet() && outputHeaders->isJSONContentType();
}


void Request::setContentType(const string &contentType) {
  getOutputHeaders().setContentType(contentType);
}


void Request::guessContentType() {
  getOutputHeaders().guessContentType(uri.getExtension());
}


void Request::outSetContentEncoding(Compression compression) {
  switch (compression) {
  case COMPRESSION_NONE: break;
  case COMPRESSION_ZLIB:
  case COMPRESSION_GZIP:
  case COMPRESSION_BZIP2:
  case COMPRESSION_LZ4:
    outSet("Content-Encoding", getContentEncoding(compression));
    break;
  case COMPRESSION_AUTO: THROW("Unexpected compression method");
  }
}


Compression Request::getRequestedCompression() const {
  if (!inHas("Accept-Encoding")) return COMPRESSION_NONE;

  vector<string> accept;
  String::tokenize(inGet("Accept-Encoding"), accept, ", \t");

  double maxQ = 0;
  double otherQ = 0;
  set<string> named;
  Compression compression = COMPRESSION_NONE;

  for (auto &_name: accept) {
    double q = 1;
    string name = String::toLower(_name);

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
      if (name == "identity")   compression = COMPRESSION_NONE;
      else if (name == "gzip")  compression = COMPRESSION_GZIP;
      else if (name == "zlib")  compression = COMPRESSION_ZLIB;
      else if (name == "bzip2") compression = COMPRESSION_BZIP2;
      else if (name == "lz4")   compression = COMPRESSION_LZ4;
      else q = 0;
    }

    if (maxQ < q) maxQ = q;
  }

  // Currently, the only standard compression format we support is gzip, so
  // if the user specifies something like "*;q=1" and doesn't give gzip
  // an explicit quality value then we select gzip compression.
  if (maxQ < otherQ && named.find("gzip") == named.end())
    compression = COMPRESSION_GZIP;

  return compression;
}


bool Request::hasCookie(const string &name) const {
  if (!inHas("Cookie")) return false;

  vector<string> cookies;
  String::tokenize(inGet("Cookie"), cookies, "; \t\n\r");

  for (auto &cookie: cookies)
    if (name == cookie.substr(0, cookie.find('='))) return true;

  return false;
}


string Request::findCookie(const string &name) const {
  if (inHas("Cookie")) {
    // Return only the first matching cookie
    vector<string> cookies;
    String::tokenize(inGet("Cookie"), cookies, "; \t\n\r");

    for (auto &cookie: cookies) {
      size_t pos = cookie.find('=');

      if (name == cookie.substr(0, pos))
        return pos == string::npos ? string() : cookie.substr(pos + 1);
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
                        bool secure, const string &sameSite) {
  outSet("Set-Cookie", Cookie(
           name, value, domain, path, expires, maxAge, httpOnly, secure,
           sameSite).toString());
}


string Request::getInput()  const {return inputBuffer.toString();}
string Request::getOutput() const {return outputBuffer.toString();}


SmartPointer<JSON::Value> Request::getInputJSON() const {
  Event::Buffer buf = inputBuffer;
  if (!buf.getLength()) return 0;
  Event::BufferStream<> stream(buf);
  return JSON::Reader(stream).parse();
}


const SmartPointer<JSON::Value> &Request::getJSONMessage() {
  if (msg.isNull()) {
    const Headers &hdrs = getInputHeaders();
    if (hdrs.hasContentType() && hdrs.isJSONContentType()) msg = getInputJSON();
  }

  return msg;
}


SmartPointer<istream> Request::getInputStream() const {
  return new Event::BufferStream<>(inputBuffer);
}


SmartPointer<ostream> Request::getOutputStream(Compression compression) {
  // Auto select compression type based on Accept-Encoding
  if (compression == COMPRESSION_AUTO) compression = getRequestedCompression();
  outSetContentEncoding(compression);

  SmartPointer<ostream> target = new Event::BufferStream<>(outputBuffer);

  if (compression == Compression::COMPRESSION_NONE) return target;

  struct FilteringOStreamWithRef : public io::filtering_ostream {
    SmartPointer<ostream> ref;
    virtual ~FilteringOStreamWithRef() {reset();}
  };

  SmartPointer<FilteringOStreamWithRef> out = new FilteringOStreamWithRef;
  pushCompression(compression, *out);
  out->ref = target;
  out->push(*target);

  return out;
}


void Request::sendJSONError(Status code, const string &msg) {
  reply(code, [&] (JSON::Sink &sink) {
    sink.beginDict();
    sink.insert("code", code);
    sink.insert("error", msg.empty() ? code.toString() : msg);
    sink.endDict();
  });
}


void Request::sendError(Status code, const string &msg) {
  if (isJSONContentType()) return sendJSONError(code, msg);

  outSet("Content-Type", "text/plain");
  outSet("Connection", "close");

  outputBuffer.clear();
  send(msg);
  reply(code);
}



void Request::sendError(Status code) {
  if (!code) code = HTTP_INTERNAL_SERVER_ERROR;
  string msg = String((int)code) + " " + code.getDescription();
  sendError(code, msg);
}


void Request::sendError(Status code, const Exception &e) {
  if (isJSONContentType()) {
    reply(code, [&] (JSON::Sink &sink) {
      sink.beginDict();
      sink.beginInsert("error");
      e.write(sink, false);
      sink.endDict();
    });

  } else sendError(code, e.getMessages());
}


void Request::sendError(const Exception &e) {
  sendError((Status::enum_t)e.getTopCode(), e);
}


void Request::sendError(const exception &e) {
  sendError((Status::enum_t)0, e.what());
}


void Request::send(function<void (JSON::Sink &sink)> cb) {
  outputBuffer.clear();

  Event::Buffer buffer;
  Event::BufferStream<> stream(buffer);
  JSON::Writer writer(stream, 0, true);

  cb(writer);

  writer.close();
  stream.flush();

  setContentType("application/json");
  send(buffer);
}


void Request::send(const Event::Buffer &buf) {outputBuffer.add(buf);}


void Request::send(const char *data, unsigned length) {
  outputBuffer.add(data, length);
}


void Request::send(const char *s) {outputBuffer.add(s);}
void Request::send(const string &s) {outputBuffer.add(s);}
void Request::sendFile(const string &path) {outputBuffer.addFile(path);}


void Request::reply(Status::enum_t code, write_cb_t cb) {
  if (replying) THROW("Request already replying");

  responseCode = code ? code : (Status::enum_t)HTTP_INTERNAL_SERVER_ERROR;
  write(cb);
  replying = true;
}


void Request::reply(const Event::Buffer &buf) {reply(HTTP_OK, buf);}


void Request::reply(const char *data, unsigned length) {
  reply(HTTP_OK, data, length);
}


void Request::reply(Status code, const char *data, unsigned length) {
  reply(code, Event::Buffer(data, length));
}


void Request::reply(Status code, function<void (JSON::Sink &sink)> cb) {
  send(cb);
  reply(code);
}


void Request::reply(function<void (JSON::Sink &sink)> cb) {reply(HTTP_OK, cb);}


void Request::reply(Status code, const Event::Buffer &buf) {
  send(buf);
  reply(code);
}


void Request::startChunked(Status code) {
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


void Request::sendChunk(function<void (JSON::Sink &sink)> cb) {
  Event::Buffer buffer;
  Event::BufferStream<> stream(buffer);
  JSON::Writer writer(stream, 0, true);

  cb(writer);

  writer.close();
  stream.flush();

  sendChunk(buffer);
}


void Request::sendChunk(const Event::Buffer &buf) {
  if (!chunked) THROW("Not chunked");

  LOG_DEBUG(4, "Sending " << buf.getLength() << " byte chunk");

  // Check for final empty chunk.  Must be before add() below
  if (!buf.getLength()) chunked = false;

  if (connection.isNull()) return; // Ignore write

  Event::Buffer out;
  out.add(String::printf("%x\r\n", buf.getLength()));
  out.add(buf);
  out.add("\r\n");

  connection->writeRequest(this, out, !chunked);
}


void Request::sendChunk(const char *data, unsigned length) {
  sendChunk(Event::Buffer(data, length));
}


void Request::endChunked() {sendChunk(Event::Buffer(""));}


void Request::redirect(const URI &uri, Status code) {
  outSet("Location", uri);
  outSet("Content-Length", "0");
  reply(code, "", 0);
}


void Request::onResponse(Event::ConnectionError error) {
  if (error) LOG_DEBUG(4, "< " << error);
  else {
    LOG_INFO(2, "< " << getResponseLine());
    LOG_DEBUG(5, getInputHeaders() << '\n');
    LOG_DEBUG(6, inputBuffer.hexdump() << '\n');
  }

  setConnectionError(error);
}


void Request::onRequest() {
  LOG_INFO(2, "< " << getRequestLine());
  LOG_DEBUG(5, getInputHeaders() << '\n');
  LOG_DEBUG(6, inputBuffer.hexdump() << '\n');
}


bool Request::mustHaveBody() const {
  return responseCode != HTTP_NO_CONTENT && responseCode != HTTP_NOT_MODIFIED &&
    (200 <= responseCode || responseCode < 100) && method != HTTP_HEAD &&
    method != HTTP_CONNECT && method != HTTP_OPTIONS;
}


bool Request::mayHaveBody() const {
  return method == HTTP_POST || method == HTTP_PUT || method == HTTP_PATCH;
}


bool Request::needsClose() const {
  return (inputHeaders.isSet() && inputHeaders->needsClose()) ||
    (outputHeaders.isSet() && outputHeaders->needsClose());
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

  responseCode = Status::parse(parts[1]);
  if (!responseCode) THROW("Bad response code " << parts[1]);

  if (parts.size() == 3) responseCodeLine = parts[2];
}


void Request::write(write_cb_t cb) {
  if (connection.isNull()) {
    if (cb) cb(false); // Ignore write
    return;
  }

  Event::Buffer out;
  writeHeaders(out);
  if (outputBuffer.getLength()) out.add(outputBuffer);

  bytesWritten += out.getLength();

  bool continueProcessing =
    !chunked && responseCode != HTTP_SWITCHING_PROTOCOLS;
  connection->writeRequest(this, out, continueProcessing, cb);
}


void Request::writeResponse(Event::Buffer &buf) {
  buf.add(getResponseLine() + "\r\n");

  if (version.getMajor() == 1) {
    if (1 <= version.getMinor() && !outHas("Date"))
      outSet("Date", Time().toString("%a, %d %b %Y %H:%M:%S GMT"));

    // If the protocol is 1.0 and connection was keep-alive add keep-alive
    bool keepAlive =
      inputHeaders.isSet() && inputHeaders->connectionKeepAlive();
    if (!version.getMinor() && keepAlive) outSet("Connection", "keep-alive");

    if ((0 < version.getMinor() || keepAlive) && mustHaveBody() &&
        !outHas("Transfer-Encoding") && !outHas("Content-Length"))
      outSet("Content-Length", String(outputBuffer.getLength()));
  }

  // Don't reply with empty JSON
  if (outputBuffer.isEmpty() && isJSONContentType())
    outRemove("Content-Type");

  // Add Content-Type
  if (mustHaveBody() && !hasContentType()) guessContentType();

  // If request asked for close, send close
  if (inputHeaders.isSet() && inputHeaders->needsClose())
    outSet("Connection", "close");

  LOG_INFO(300 <= responseCode ? 1 : 4, "> " << getResponseLine());
  LOG_DEBUG(5, getOutputHeaders() << '\n');
  LOG_DEBUG(6, outputBuffer.hexdump() << '\n');
}


void Request::writeRequest(Event::Buffer &buf) {
  // Generate request line
  buf.add(getRequestLine() + "\r\n");

  if (method == HTTP_CONNECT) {
    if (!outHas("Host"))
      outSet("Host", uri.getHost() + ":" + String(uri.getPort()));

  } else {
    if (!outHas("Host")) outSet("Host", uri.getHost());
    if (!outHas("Connection")) outSet("Connection", "close"); // Don't persist
    if (!outHas("Accept")) outSet("Accept", "*/*");
  }

  // Add missing content length if may have body
  if (mayHaveBody() && !outHas("Content-Length"))
    outSet("Content-Length", String(outputBuffer.getLength()));

  LOG_INFO(2, "> " << getRequestLine());
  LOG_DEBUG(5, getOutputHeaders() << '\n');
  LOG_DEBUG(6, outputBuffer.hexdump() << '\n');
}


void Request::writeHeaders(Event::Buffer &buf) {
  if (connection->isIncoming()) writeResponse(buf);
  else writeRequest(buf);

  if (outputHeaders.isSet())
    for (auto it: *outputHeaders)
      if (!it.value().empty())
        buf.add(String::printf(
          "%s: %s\r\n", it.key().c_str(), it.value().c_str()));

  buf.add("\r\n");
}
