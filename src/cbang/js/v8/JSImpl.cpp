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

#include "JSImpl.h"
#include "Context.h"
#include "Factory.h"
#include "Value.h"

#include <cbang/js/Javascript.h>
#include <cbang/util/SmartFunctor.h>

#include <libplatform/libplatform.h>

using namespace cb::gv8;
using namespace cb;
using namespace std;


JSImpl *JSImpl::singleton = 0;


JSImpl::JSImpl(js::Javascript &js) {
  if (singleton) THROW("There can be only one. . .");
  singleton = this;

  v8::Isolate::CreateParams params;
  params.array_buffer_allocator =
    v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate = v8::Isolate::New(params);

  scope = new Scope(isolate);
  ctx = new Context(isolate);
}


JSImpl::~JSImpl() {
  ctx.release();
  scope.release();
  isolate->Dispose();
  singleton = 0;
}


void JSImpl::init(int *argc, char *argv[]) {
  if (argv) v8::V8::SetFlagsFromCommandLine(argc, argv, true);
  static std::unique_ptr<v8::Platform> platform =
    v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
}


JSImpl &JSImpl::current() {
  if (!singleton) THROW("No instance created");
  return *singleton;
}


SmartPointer<js::Factory> JSImpl::getFactory() {return new Factory;}
SmartPointer<js::Scope> JSImpl::enterScope() {return new Context::Scope(ctx);}


SmartPointer<js::Scope> JSImpl::newScope() {
  return new Context::Scope(new Context(isolate));
}


void JSImpl::interrupt() {isolate->TerminateExecution();}


SmartPointer<js::StackTrace> JSImpl::getStackTrace(unsigned maxFrames) {
  v8::StackTrace::StackTraceOptions options =
    static_cast<v8::StackTrace::StackTraceOptions>(
      v8::StackTrace::kOverview |
      v8::StackTrace::kExposeFramesAcrossSecurityOrigins);
  v8::Local<v8::StackTrace> v8Trace =
    v8::StackTrace::CurrentStackTrace(isolate, maxFrames, options);
  SmartPointer<js::StackTrace> trace = new js::StackTrace;

  for (int i = 0; i < v8Trace->GetFrameCount(); i++) {
    v8::Local<v8::StackFrame> frame = v8Trace->GetFrame(isolate, i);
    auto v8Filename = frame->GetScriptName();
    auto v8Function = frame->GetFunctionName();
    string filename =
      v8Filename.IsEmpty() ? string() : gv8::Value(v8Filename).toString();
    string function =
      v8Function.IsEmpty() ? string() : gv8::Value(v8Function).toString();

    trace->push_back(FileLocation(filename, function, frame->GetLineNumber(),
                                  frame->GetColumn()));
  }

  return trace;
}
