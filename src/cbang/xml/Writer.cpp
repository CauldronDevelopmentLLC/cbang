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

#include "Writer.h"

#include <cctype>

using namespace std;
using namespace cb;
using namespace cb::XML;


void Writer::entityRef(const string &name) {
  stream << '&' << name << ';';
  startOfLine = false;
}


void Writer::startElement(const string &name, const Attributes &attrs) {
  startOfLine = false; // Always start element on a new line

  if (!closed) stream << '>';
  if (depth) wrap();

  stream << '<' << escape(name);

  for (auto &p: attrs)
    stream << ' ' << escape(p.first) << "='" << escape(p.second) << '\'';

  closed = false;
  depth++;
}


void Writer::endElement(const string &name) {
  depth--;

  if (!closed) {
    stream << "/>";
    closed = true;

  } else {
    wrap();
    stream << "</" << escape(name) << '>';
  }

  startOfLine = false;
}


void Writer::text(const string &text) {
  if (!text.length()) return;

  auto it = text.begin();

  if (!closed) {
    stream << '>';
    closed = true;
    startOfLine = false;

    if (pretty) while (it != text.end() && isspace(*it)) it++;
    wrap();
  }

  for (; it != text.end(); it++) {
    if (dontEscapeText) stream << *it;
    else
      switch (*it) {
      case '<': stream << "&lt;"; break;
      case '>': stream << "&gt;"; break;
      case '&': stream << "&amp;"; break;
      default: stream << *it; break;
      }

    startOfLine = *it == '\n' || *it == '\r';
  }

  // TODO wrap lines at 80 columns in pretty print mode?
}


void Writer::cdata(const string &data) {
  if (!data.length()) return;

  if (!closed) {
    stream << '>';
    closed = true;
    startOfLine = false;
  }

  stream << "<![CDATA[" << data << "]]>";
}


void Writer::comment(const string &text) {
  startOfLine = false; // Always start comment on a new line

  if (!closed) stream << '>';

  wrap();
  stream << "<!-- ";

  bool dash = false;
  for (auto c: text) {
    if (c == '-') {
      if (dash) {
        // Drop double dash
        stream.put(' ');
        dash = false;
        continue;
      }
      dash = true;
    } else dash = false;

    switch (c) {
    case '\r': break;
#ifdef _WIN32
    case '\n': stream.put('\r');
#endif
    default: stream.put(c);
    }
  }

  stream << " -->";
  closed = true;
  startOfLine = false;
}


void Writer::indent() {
  if (pretty) {
    for (unsigned i = 0; i < depth; i++)
      stream << "  ";
    startOfLine = false;
  }
}


void Writer::wrap() {
  if (pretty) {
    if (!startOfLine) {
#ifdef _WIN32
      stream << "\r\n";
#else
      stream << '\n';
#endif
    }

    indent();
  }
}


const string Writer::escape(const string &name) {
  string result;

  for (auto c: name)
    switch (c) {
    case '<':  result += "&lt;";   break;
    case '>':  result += "&gt;";   break;
    case '&':  result += "&amp;";  break;
    case '"':  result += "&quot;"; break;
    case '\'': result += "&apos;"; break;
    case '\r': break;
#ifdef _WIN32
    case '\n': result += "\r\n";   break;
#endif
    default:   result += c;        break;
    }

  return result;
}


void Writer::simpleElement(
  const string &name, const string &content, const Attributes &attrs) {
  startElement(name, attrs);
  text(content);
  endElement(name);
}
