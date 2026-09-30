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

#include "Response.h"

#include <cbang/String.h>
#include <cbang/http/Request.h>
#include <cbang/json/Reader.h>
#include <cbang/json/List.h>

using namespace std;
using namespace cb;
using namespace cb::ClickHouse;


namespace {
  // Strip the "Code: 62. DB::Exception: " prefix and " (version ...)"
  // suffix from a ClickHouse error, leaving the message and error name
  string parseError(const string &body) {
    string msg = String::trim(body);

    size_t start = msg.find("DB::Exception: ");
    if (start != string::npos) msg = msg.substr(start + 15);

    size_t end = msg.rfind(" (version ");
    if (end != string::npos) msg = msg.substr(0, end);

    return msg;
  }
}


Response::Response(const HTTP::Request &req) {
  if (req.getConnectionError()) {
    error = SSTR("Connection failed: "
                 << req.getConnectionError().getDescription());
    return;
  }

  status = req.getResponseCode();

  if (req.inHas("X-ClickHouse-Exception-Code"))
    code = String::parseU32(req.inFind("X-ClickHouse-Exception-Code"));

  string body = req.getInput();

  if (status != HTTP_OK || code) {
    error = parseError(body);
    if (error.empty()) error = status.toString();

  } else if (!String::trim(body).empty()) // Empty for non-query statements
    try {
      json = JSON::Reader::parse(body);
    } catch (const Exception &e) {
      error = "Invalid JSON response: " + e.getMessage();
    }
}


bool Response::isDataError() const {
  // ClickHouse could not parse the data or it broke a constraint, so sending
  // it again would fail the same way.  Not all of these reply with a 4xx.
  switch (code) {
  case 6:   // CANNOT_PARSE_TEXT
  case 7:   // INCORRECT_NUMBER_OF_COLUMNS
  case 16:  // NO_SUCH_COLUMN_IN_TABLE
  case 19:  // SIZE_OF_FIXED_STRING_DOESNT_MATCH
  case 25:  // CANNOT_PARSE_ESCAPE_SEQUENCE
  case 26:  // CANNOT_PARSE_QUOTED_STRING
  case 27:  // CANNOT_PARSE_INPUT_ASSERTION_FAILED
  case 33:  // CANNOT_READ_ALL_DATA
  case 38:  // CANNOT_PARSE_DATE
  case 41:  // CANNOT_PARSE_DATETIME
  case 53:  // TYPE_MISMATCH
  case 69:  // ARGUMENT_OUT_OF_BOUND
  case 70:  // CANNOT_CONVERT_TYPE
  case 72:  // CANNOT_PARSE_NUMBER
  case 117: // INCORRECT_DATA
  case 120: // CANNOT_INSERT_VALUE_OF_DIFFERENT_SIZE_INTO_TUPLE
  case 128: // TOO_LARGE_ARRAY_SIZE
  case 130: // CANNOT_READ_ARRAY_FROM_TEXT
  case 131: // TOO_LARGE_STRING_SIZE
  case 190: // SIZES_OF_ARRAYS_DONT_MATCH
  case 321: // VALUE_IS_OUT_OF_RANGE_OF_DATA_TYPE
  case 349: // CANNOT_INSERT_NULL_IN_ORDINARY_COLUMN
  case 376: // CANNOT_PARSE_UUID
  case 407: // DECIMAL_OVERFLOW
  case 441: // CANNOT_PARSE_DOMAIN_VALUE_FROM_STRING
  case 467: // CANNOT_PARSE_BOOL
  case 469: // VIOLATED_CONSTRAINT
  case 563: // CANNOT_READ_MAP_FROM_TEXT
  case 626: // CANNOT_SKIP_UNKNOWN_FIELD
  case 632: // UNEXPECTED_DATA_AFTER_PARSED_VALUE
  case 675: // CANNOT_PARSE_IPV4
  case 676: // CANNOT_PARSE_IPV6
  case 691: // UNKNOWN_ELEMENT_OF_ENUM
    return true;

  default: return false;
  }
}


JSON::ValuePtr Response::getMeta() const {
  return json.isSet() && json->hasList("meta") ?
    json->get("meta") : new JSON::List;
}


JSON::ValuePtr Response::getData() const {
  return json.isSet() && json->hasList("data") ?
    json->get("data") : new JSON::List;
}


string Response::getMessage() const {
  if (code) return SSTR("ClickHouse:" << code << ": " << error);
  return "ClickHouse: " + error;
}
