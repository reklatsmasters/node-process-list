/**
 * Copyright (c) 2014 Dmitry Tsvettsikh <https://github.com/reklatsmasters>
 *
 * MIT License <https://github.com/reklatsmasters/node-process-list/blob/master/LICENSE>
 */

#include "snapshot.h"  // NOLINT(build/include)

#include <nan.h>

#include <algorithm>
#include <memory>

#include "tasklist.h"  // NOLINT(build/include)

using v8::Number;
using v8::String;
using v8::Array;
using v8::Object;
using v8::Array;
using v8::Function;
using v8::Local;
using v8::Value;
using v8::Date;
using pl::process_fields;

#define STR(s) Nan::New<v8::String>(s).ToLocalChecked()

#define PROP_BOOL(obj, prop) \
  Nan::To<bool>(Nan::Get(obj, STR(prop)).ToLocalChecked()).FromJust()

class SnapshotWorker : public Nan::AsyncWorker {
 public:
  SnapshotWorker(Nan::Callback *callback, const struct process_fields &fields)
  : Nan::AsyncWorker(callback), psfields(fields) {
  }

  ~SnapshotWorker() {}

  void Execute() {
    try {
      tasks = pl::list(psfields);
    } catch(const std::exception &e) {
      SetErrorMessage(e.what());
    }
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;

    Local<Array> jobs = Nan::New<Array>(tasks.size());

    for (uint32_t i = 0; i < jobs->Length(); ++i) {
      Local<Object> hash = Nan::New<Object>();

      if (psfields.name) {
        Nan::Set(hash, STR("name"), STR(tasks.at(i).name));
      }

      if (psfields.pid) {
        Nan::Set(hash, STR("pid"), Nan::New<Number>(tasks.at(i).pid));
      }

      if (psfields.ppid) {
        Nan::Set(hash, STR("ppid"), Nan::New<Number>(tasks.at(i).ppid));
      }

      if (psfields.path) {
        Nan::Set(hash, STR("path"), STR(tasks.at(i).path));
      }

      if (psfields.threads) {
        Nan::Set(hash, STR("threads"),
          Nan::New<Number>(tasks.at(i).threads));
      }

      if (psfields.owner) {
        Nan::Set(hash, STR("owner"), STR(tasks.at(i).owner));
      }

      if (psfields.priority) {
        Nan::Set(hash, STR("priority"),
          Nan::New<Number>(tasks.at(i).priority));
      }

      if (psfields.cmdline) {
        Nan::Set(hash, STR("cmdline"), STR(tasks.at(i).cmdline));
      }

      if (psfields.starttime) {
        Nan::Set(hash, STR("starttime"),
          Nan::New<Date>(tasks.at(i).starttime).ToLocalChecked());
      }

      if (psfields.vmem) {
        Nan::Set(hash, STR("vmem"), STR(std::to_string(tasks.at(i).vmem)));
      }

      if (psfields.pmem) {
        Nan::Set(hash, STR("pmem"), STR(std::to_string(tasks.at(i).pmem)));
      }

      if (psfields.cpu) {
        Nan::Set(hash, STR("cpu"),
          Nan::New<Number>(tasks.at(i).cpu));
      }

      if (psfields.utime) {
        Nan::Set(hash, STR("utime"), STR(std::to_string(tasks.at(i).utime)));
      }

      if (psfields.stime) {
        Nan::Set(hash, STR("stime"), STR(std::to_string(tasks.at(i).stime)));
      }

      Nan::Set(jobs, i, hash);
    }

    Local<Value> argv[] = {
      Nan::Null(),
      jobs
    };

    callback->Call(2, argv, async_resource);
  }

  void HandleErrorCallback() {
    Nan::HandleScope scope;

    Local<Value> argv[] = {
      Nan::Error(ErrorMessage())
    };

    callback->Call(1, argv, async_resource);
  }

 private:
  pl::list_t tasks;
  process_fields psfields;
};

NAN_METHOD(snapshot) {
  auto arg0 = info[0].As<Object>();

  struct process_fields fields = {
    PROP_BOOL(arg0, "pid"),
    PROP_BOOL(arg0, "ppid"),
    PROP_BOOL(arg0, "path"),
    PROP_BOOL(arg0, "name"),
    PROP_BOOL(arg0, "owner"),
    PROP_BOOL(arg0, "cmdline"),
    PROP_BOOL(arg0, "threads"),
    PROP_BOOL(arg0, "priority"),
    PROP_BOOL(arg0, "starttime"),
    PROP_BOOL(arg0, "vmem"),
    PROP_BOOL(arg0, "pmem"),
    PROP_BOOL(arg0, "cpu"),
    PROP_BOOL(arg0, "utime"),
    PROP_BOOL(arg0, "stime")
  };

  auto *callback = new Nan::Callback(info[1].As<Function>());

  Nan::AsyncQueueWorker(new SnapshotWorker(callback, fields));
}
