/******************************************************************************\

         This file is part of the C! library.  A.K.A the cbang library.

               Copyright (c) 2003-2018, Cauldron Development LLC
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

#include "YAMLReader.h"

#include "Builder.h"

#include <cbang/Math.h>
#include <cbang/io/StringInputSource.h>
#include <cbang/util/Regex.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/log/Logger.h>

#include <yaml.h>

using namespace std;
using namespace cb;
using namespace cb::JSON;


namespace {
  int _yaml_read_handler(void *data, unsigned char *buffer, size_t size,
                         size_t *size_read) {
    istream *stream = (istream *)data;

    if (stream->eof()) {
      *size_read = 0;
      return 1;
    }

    if (stream->fail()) return 0;

    stream->read((char *)buffer, size);
    *size_read = stream->gcount();

    return 1;
  }


  const char *_yaml_event_type_str(yaml_event_type_t t) {
    switch (t) {
    case YAML_NO_EVENT:             return "NO";
    case YAML_STREAM_START_EVENT:   return "STREAM_START";
    case YAML_STREAM_END_EVENT:     return "STREAM_END";
    case YAML_DOCUMENT_START_EVENT: return "DOCUMENT_START";
    case YAML_DOCUMENT_END_EVENT:   return "DOCUMENT_END";
    case YAML_ALIAS_EVENT:          return "ALIAS";
    case YAML_SCALAR_EVENT:         return "SCALAR";
    case YAML_SEQUENCE_START_EVENT: return "SEQUENCE_START";
    case YAML_SEQUENCE_END_EVENT:   return "SEQUENCE_END";
    case YAML_MAPPING_START_EVENT:  return "MAPPING_START";
    case YAML_MAPPING_END_EVENT:    return "MAPPING_END";
    }
    return "INVALID";
  }
}


class YAMLReader::Private {
public:
  yaml_parser_t parser;


  Private(istream &stream) {
    if (!yaml_parser_initialize(&parser))
      THROW("Failed to initialize YAML parser");

    yaml_parser_set_input(&parser, _yaml_read_handler, &stream);
  }


  ~Private() {yaml_parser_delete(&parser);}


  void parse(yaml_event_t &event) {
    if (!yaml_parser_parse(&parser, &event))
      THROWS("Parser error " << parser.error << ": " << parser.problem);
  }
};


// See YAML spec: http://yaml.org/spec/1.2/spec.html#id2805071
Regex YAMLReader::nullRE("([Nn]ull)|(NULL)|~|()");
Regex YAMLReader::boolRE("([Tt]rue)|(TRUE)|([Ff]alse)|(FALSE)");
Regex YAMLReader::intRE("([-+]?[0-9]+)|(0o[0-7]+)|(0x[0-9a-fA-F]+)");
Regex YAMLReader::floatRE("[-+]?((\\.[0-9]+)|([0-9]+(\\.[0-9]*)?))"
                          "([eE][-+]?[0-9]+)?");
Regex YAMLReader::infRE("[-+]?\\.(([Ii]nf)|(INF))");
Regex YAMLReader::nanRE("\\.(([Nn]an)|(NAN))");


YAMLReader::YAMLReader(const InputSource &src) :
  src(src), pri(new Private(src.getStream())) {}


void YAMLReader::parse(Sink &sink) {
  vector<yaml_event_type_t> stack;
  yaml_event_t event;
  bool haveKey = false;

  while (true) {
    pri->parse(event);

    LOG_DEBUG(5, "YAML: " << _yaml_event_type_str(event.type));

    switch (event.type) {
    case YAML_NO_EVENT: THROW("YAML No event");
    case YAML_STREAM_START_EVENT: break;

    case YAML_STREAM_END_EVENT: yaml_event_delete(&event); return;

    case YAML_DOCUMENT_START_EVENT: break;
    case YAML_DOCUMENT_END_EVENT: yaml_event_delete(&event); return;

    case YAML_SEQUENCE_START_EVENT:
      if (!stack.empty() && stack.back() == YAML_SEQUENCE_START_EVENT)
        sink.beginAppend();
      sink.beginList();
      stack.push_back(event.type);
      haveKey = false;
      break;

    case YAML_SEQUENCE_END_EVENT:
      if (stack.empty() || stack.back() != YAML_SEQUENCE_START_EVENT)
        THROW("Invalid YAML end sequence");
      stack.pop_back();
      sink.endList();
      break;

    case YAML_MAPPING_START_EVENT:
      if (!stack.empty() && stack.back() == YAML_SEQUENCE_START_EVENT)
        sink.beginAppend();
      sink.beginDict();
      stack.push_back(event.type);
      haveKey = false;
      break;

    case YAML_MAPPING_END_EVENT:
      if (stack.empty() || stack.back() != YAML_MAPPING_START_EVENT)
        THROW("Invalid YAML end mapping");
      stack.pop_back();
      sink.endDict();
      break;

    case YAML_ALIAS_EVENT: break; // Ignored

    case YAML_SCALAR_EVENT: {
      string value = string((const char *)event.data.scalar.value,
                            event.data.scalar.length);

      if (!stack.empty()) {
        if (stack.back() == YAML_SEQUENCE_START_EVENT) sink.beginAppend();

        else if (!haveKey) {
          sink.beginInsert(value);
          haveKey = true;
          yaml_event_delete(&event);
          continue;

        } else haveKey = false;
      }

      string tag = event.data.scalar.tag ?
        string((const char *)event.data.scalar.tag) : "";

      LOG_DEBUG(5, "YAML: tag=" << tag);

      if (!tag.empty() && !event.data.scalar.plain_implicit) {
        if (tag == YAML_INT_TAG) sink.write(String::parseS64(value));
        else if (tag == YAML_FLOAT_TAG)
          sink.write(String::parseDouble(value));
        else if (tag == YAML_BOOL_TAG)
          sink.writeBoolean(String::parseBool(value));
        else if (tag == YAML_NULL_TAG) sink.writeNull();
        else if (tag == "!include")
          YAMLReader(SystemUtilities::absolute(src.getName(), value))
            .parse(sink);
        else sink.write(value);

      } else {
        // Resolve implicit tags
        if (event.data.scalar.quoted_implicit) sink.write(value);
        else if (nullRE.match(value)) sink.writeNull();
        else if (boolRE.match(value))
          sink.writeBoolean(String::parseBool(value));

        else if (intRE.match(value)) {
          if (value[0] == '-') sink.write(String::parseS64(value));
          else sink.write(String::parseU64(value));

        } else if (floatRE.match(value))
          sink.write(String::parseDouble(value));

        else if (infRE.match(value)) {
          if (value[0] == '-') sink.write(-INFINITY);
          else sink.write(INFINITY);

        } else if (nanRE.match(value)) sink.write(NAN);

        else sink.write(value);
      }

      break;
    }
    }

    yaml_event_delete(&event);
  }
}


ValuePtr YAMLReader::parse() {
  Builder builder;
  parse(builder);
  return builder.getRoot();
}


SmartPointer<Value> YAMLReader::parse(const InputSource &src) {
  return YAMLReader(src).parse();
}


SmartPointer<Value> YAMLReader::parseString(const string &s) {
  return parse(StringInputSource(s));
}


void YAMLReader::parse(docs_t &docs) {
  while (true) {
    ValuePtr doc = parse();
    if (doc.isNull()) break;
    docs.push_back(doc);
  }
}


void YAMLReader::parse(const InputSource &src, docs_t &docs) {
  YAMLReader(src).parse(docs);
}


void YAMLReader::parseString(const string &s, docs_t &docs) {
  parse(StringInputSource(s), docs);
}
