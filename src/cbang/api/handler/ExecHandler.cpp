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

#include "ExecHandler.h"
#include "ExecProcess.h"

#include <cbang/api/API.h>
#include <cbang/api/Blob.h>
#include <cbang/api/Resolver.h>
#include <cbang/os/Subprocess.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/os/TemporaryDirectory.h>
#include <cbang/json/Dict.h>
#include <cbang/json/String.h>

#include <filesystem>

using namespace std;
using namespace cb;
using namespace cb::API;


namespace {
  // Write binary envelope values to files in ``dir`` and substitute their
  // paths, recursing through dicts and lists
  void writeBlobs(JSON::Value &value, const string &dir) {
    auto writeOne = [&] (const string &name, const Blob &blob) {
      string path = dir + "/" + name;
      for (unsigned i = 0; SystemUtilities::exists(path); i++)
        path = dir + "/" + name + "." + String(i);

      auto &data = blob.getData();
      SystemUtilities::oopen(path)->write(data.data(), data.length());
      return path;
    };

    if (value.isDict()) {
      vector<string> keys;
      for (auto e: value.entries()) keys.push_back(e.key());

      for (auto &key: keys) {
        auto v = value.get(key);
        auto *blob = dynamic_cast<Blob *>(v.get());
        if (blob) value.insert(key, writeOne(key, *blob));
        else writeBlobs(*v, dir);
      }

    } else if (value.isList())
      for (unsigned i = 0; i < value.size(); i++) {
        auto v = value.get(i);
        auto *blob = dynamic_cast<Blob *>(v.get());
        if (blob) value.set(i, new JSON::String(writeOne(String(i), *blob)));
        else writeBlobs(*v, dir);
      }
  }
}


ExecHandler::ExecHandler(API &api, const JSON::ValuePtr &config) :
  api(api) {

  JSON::ValuePtr cmdVal;

  if (config->isString()) cmdVal = config;
  else {
    if (!config->isDict()) THROW("Invalid exec config");
    cmdVal = config->get("cmd");
    if (config->has("input")) input = config->get("input");
  }

  if (cmdVal->isString()) Subprocess::parse(cmdVal->asString(), cmd);
  else if (cmdVal->isList()) for (auto &v: *cmdVal) cmd.push_back(v->asString());
  else THROW("exec 'cmd' must be a string or list");

  // Default envelope sends the request args
  if (input.isNull()) {
    input = new JSON::Dict;
    input->insert("args", new JSON::String("{args}"));
  }
}


void ExecHandler::operator()(const CtxPtr &ctx, const Cont &next) {
  auto resolver = ctx->getResolver();

  // Resolve the command and input envelope against the request
  vector<string> rcmd;
  for (auto &arg: cmd) rcmd.push_back(resolver->resolve(arg));

  auto in = input->copy(true);
  resolver->resolve(*in);

  // A private temp dir for this step, exported to the process as TMPDIR and
  // removed when the step completes.  Binary envelope values are written to
  // files there and replaced by their paths.
  auto tmpDir = SmartPtr(new TemporaryDirectory(
    std::filesystem::temp_directory_path().string()));
  writeBlobs(*in, tmpDir->getPath());

  auto &pool = api.getProcPool();
  pool.enqueue(
    new ExecProcess(pool.getBase(), rcmd, in->toString(), tmpDir, ctx, next));
}
