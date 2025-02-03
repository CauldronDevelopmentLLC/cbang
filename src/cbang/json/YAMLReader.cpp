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

#include "YAMLReader.h"

#include "Builder.h"
#include "TeeSink.h"
#include "YAMLMergeSink.h"
#include "Dict.h"

#include <cbang/util/Regex.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/log/Logger.h>

#include <cmath>

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
  string name;


  Private(const InputSource &src) : name(src.getName()) {
    if (!yaml_parser_initialize(&parser))
      THROW("Failed to initialize YAML parser");

    yaml_parser_set_input(&parser, _yaml_read_handler, &(std::istream &)src);
  }


  ~Private() {yaml_parser_delete(&parser);}


  FileLocation location(const yaml_mark_t &mark) const {
    return FileLocation(name, mark.line, mark.column);
  }


  FileLocation location() const {return location(parser.mark);}


  void parse(yaml_event_t &event) {
    if (!yaml_parser_parse(&parser, &event))
      throw ParseError(
        SSTR("YAML: " << parser.problem), parser.error,
        location(parser.problem_mark));
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
  src(src), pri(new Private(src)) {}


void YAMLReader::parse(Sink &sink) {
  try {
    _parse(sink);

  } catch (const Exception &e) {
    if (dynamic_cast<const ParseError *>(&e)) throw;
    throw ParseError(SSTR("YAML: " << e.getMessage()), pri->location(), e);
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


SmartPointer<Value> YAMLReader::parseFile(const string &path) {
  return parse(InputSource::open(path));
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


void YAMLReader::parseFile(const string &path, docs_t &docs) {
  parse(InputSource::open(path), docs);
}


void YAMLReader::_parse(Sink &sink) {
  struct Frame {
    yaml_event_type_t event;
    string anchor;

    Frame(yaml_event_type_t event, const string &anchor = string()) :
      event(event), anchor(anchor) {}
  };

  vector<Frame> stack;
  JSON::Dict anchors;
  yaml_event_t event;
  bool haveKey = false;
  SmartPointer<Sink> target = PhonyPtr(&sink);

  auto close_merge =
    [&] () {
      if (target.isInstance<YAMLMergeSink>()) {
        auto sink = target.cast<YAMLMergeSink>();
        if (!sink->getDepth()) {
          target = sink->getTarget();
          LOG_DEBUG(5, "YAML: merge closed");
        }
      }
    };

  auto close_anchor =
    [&] () {
      if (!stack.back().anchor.empty()) {
        auto tee = target.cast<TeeSink>();
        auto builder = tee->getRight().cast<Builder>();
        anchors.insert(stack.back().anchor, builder->getRoot());
        target = tee->getLeft();
        LOG_DEBUG(5, "YAML: anchor '" << stack.back().anchor << "' closed");
      }
    };

  while (true) {
    pri->parse(event);

    LOG_DEBUG(5, "YAML: " << _yaml_event_type_str(event.type));

    // Handle anchors & tags
    yaml_char_t *_anchor = 0;
    yaml_char_t *_tag = 0;
    switch (event.type) {
    case YAML_SEQUENCE_START_EVENT:
      _anchor = event.data.sequence_start.anchor;
      _tag = event.data.sequence_start.tag;
      break;

    case YAML_MAPPING_START_EVENT:
      _anchor = event.data.mapping_start.anchor;
      _tag = event.data.mapping_start.tag;
      break;

    case YAML_SCALAR_EVENT:
      _anchor = event.data.scalar.anchor;
      _tag = event.data.scalar.tag;
      break;

    default: break;
    }

    string tag;
    if (_tag) {
      tag = (const char *)_tag;
      LOG_DEBUG(5, "YAML: scalar tag=" << tag);
    }

    string anchor;
    if (_anchor) {
      anchor = (const char *)_anchor;
      LOG_DEBUG(5, "YAML: anchor=" << anchor);
    }

    Frame *frame = stack.empty() ? 0 : &stack.back();

    // Begin append
    if (frame && frame->event == YAML_SEQUENCE_START_EVENT)
      switch (event.type) {
      case YAML_SEQUENCE_START_EVENT:
      case YAML_MAPPING_START_EVENT:
      case YAML_SCALAR_EVENT:
      case YAML_ALIAS_EVENT:
        target->beginAppend();
        break;

      default: break;
      }

    // Must be after begin append
    if (!anchor.empty()) target = new TeeSink(target, new Builder);

    switch (event.type) {
    case YAML_NO_EVENT: PARSE_ERROR("YAML No event");
    case YAML_STREAM_START_EVENT: yaml_event_delete(&event); continue;

    case YAML_STREAM_END_EVENT: yaml_event_delete(&event); return;

    case YAML_DOCUMENT_START_EVENT: yaml_event_delete(&event); continue;
    case YAML_DOCUMENT_END_EVENT: yaml_event_delete(&event); return;

    case YAML_SEQUENCE_START_EVENT:
      target->beginList();
      stack.push_back(Frame(event.type, anchor));
      haveKey = false;
      break;

    case YAML_SEQUENCE_END_EVENT:
      if (!frame || frame->event != YAML_SEQUENCE_START_EVENT)
        PARSE_ERROR("Invalid YAML end sequence");

      target->endList();
      close_anchor();
      stack.pop_back();
      break;

    case YAML_MAPPING_START_EVENT:
      target->beginDict();
      stack.push_back(Frame(event.type, anchor));
      haveKey = false;
      break;

    case YAML_MAPPING_END_EVENT:
      if (!frame || frame->event != YAML_MAPPING_START_EVENT)
        PARSE_ERROR("Invalid YAML end mapping");

      target->endDict();
      close_anchor();
      stack.pop_back();
      break;

    case YAML_ALIAS_EVENT: {
      string anchor = (const char *)event.data.alias.anchor;
      auto it = anchors.find(anchor);
      if (!it) PARSE_ERROR("Invalid anchor '" << anchor << "'");

      (*it)->write(*target);
      haveKey = false;
      break;
    }

    case YAML_SCALAR_EVENT: {
      string value = string((const char *)event.data.scalar.value,
                            event.data.scalar.length);

      if (anchors.size() && value.find('%') != string::npos)
        value = anchors.format(value);

      if (frame && frame->event == YAML_MAPPING_START_EVENT && !haveKey) {
        // Handle special merge key but allow it to be quoted
        if (value == "<<" && !event.data.scalar.quoted_implicit) {
          target = new YAMLMergeSink(target);
          LOG_DEBUG(5, "YAML: merge");
          haveKey = true;
          yaml_event_delete(&event);
          continue;

        } else {
          // Mapping key
          if (target->has(value))
            PARSE_ERROR("Key '" << value << "' already in mapping");
          target->beginInsert(value);
          haveKey = true;
          yaml_event_delete(&event);
          continue;
        }
      }

      if (!tag.empty() && !event.data.scalar.plain_implicit) {
        if (tag == YAML_INT_TAG) target->write(String::parseS64(value));
        else if (tag == YAML_FLOAT_TAG)
          target->write(String::parseDouble(value));
        else if (tag == YAML_BOOL_TAG)
          target->writeBoolean(String::parseBool(value));
        else if (tag == YAML_NULL_TAG) target->writeNull();

        else if (tag == "!include-raw") {
          string path = SystemUtilities::absolute(src.getName(), value);
          LOG_DEBUG(5, "YAML: !include-raw " << path);
          target->write(SystemUtilities::read(path));

        } else if (tag == "!include") {
          string path = SystemUtilities::absolute(src.getName(), value);
          LOG_DEBUG(5, "YAML: !include " << path);
          YAMLReader reader(InputSource::open(path));
          reader.parse(*target);

        } else target->write(value);

      } else {
        // Resolve implicit tags
        if (event.data.scalar.quoted_implicit) target->write(value);
        else if (nullRE.match(value)) target->writeNull();
        else if (boolRE.match(value))
          target->writeBoolean(String::parseBool(value));

        else if (intRE.match(value)) {
          if (value[0] == '-') target->write(String::parseS64(value));
          else target->write(String::parseU64(value));

        } else if (floatRE.match(value))
          target->write(String::parseDouble(value));

        else if (infRE.match(value)) {
          if (value[0] == '-') target->write(-INFINITY);
          else target->write(INFINITY);

        } else if (nanRE.match(value)) target->write(NAN);

        else target->write(value);
      }

      // Close scaler anchor
      if (!anchor.empty()) {
        auto tee = target.cast<TeeSink>();
        auto builder = tee->getRight().cast<Builder>();
        anchors.insert(anchor, builder->getRoot());
        target = tee->getLeft();
      }
      break;
    }
    }

    close_merge();
    haveKey = false;
    yaml_event_delete(&event);
  }
}
