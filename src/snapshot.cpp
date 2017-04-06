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

#define STR(s) Nan::New<v8::String>(s).ToLocalChecked()

#define PROP_BOOL(obj, prop) \
  Nan::To<bool>(Nan::Get(obj, STR(prop)).ToLocalChecked()).FromJust()

struct ProcessFields {
  bool pid;
  bool ppid;
  bool name;
  bool path;
  bool owner;
  bool threads;
  bool priority;
};

class SnapshotWorker : public Nan::AsyncWorker {
 public:
  SnapshotWorker(Nan::Callback *callback, const struct ProcessFields &fields)
  : Nan::AsyncWorker(callback), psfields(fields) {
  }

  ~SnapshotWorker() {}

  void Execute() {
    tasks = pl::task::list();
  }

  void HandleOKCallback() {
    Nan::HandleScope scope;

    Local<Array> jobs = Nan::New<Array>(tasks.size());

    for (uint32_t i = 0; i < jobs->Length(); ++i) {
      Local<Object> hash = Nan::New<Object>();

      if (psfields.name) {
        Nan::Set(hash, STR("name"), STR(tasks.at(i)->name()));
      }

      if (psfields.pid) {
        Nan::Set(hash, STR("pid"), Nan::New<Number>(tasks.at(i)->pid()));
      }

      if (psfields.ppid) {
        Nan::Set(hash, STR("ppid"), Nan::New<Number>(tasks.at(i)->parentPid()));
      }

      if (psfields.path) {
        Nan::Set(hash, STR("path"), STR(tasks.at(i)->path()));
      }

      if (psfields.threads) {
        Nan::Set(hash, STR("threads"),
          Nan::New<Number>(tasks.at(i)->threads()));
      }

      if (psfields.owner) {
        Nan::Set(hash, STR("owner"), STR(tasks.at(i)->owner()));
      }

      if (psfields.priority) {
        Nan::Set(hash, STR("priority"),
          Nan::New<Number>(tasks.at(i)->priority()));
      }

      Nan::Set(jobs, i, hash);
    }

    Local<Value> argv[] = {
      Nan::Null(),
      jobs
    };

    callback->Call(2, argv);
  }

  void HandleErrorCallback() {
    Nan::HandleScope scope;

    Local<Value> argv[] = {
      Nan::Error("internal error")
    };

    callback->Call(1, argv);
  }

 private:
  pl::task::list_t tasks;
  ProcessFields psfields;
};

NAN_METHOD(snapshot) {
  auto arg0 = info[0].As<Object>();

  ProcessFields fields = {
    PROP_BOOL(arg0, "pid"),
    PROP_BOOL(arg0, "ppid"),
    PROP_BOOL(arg0, "name"),
    PROP_BOOL(arg0, "path"),
    PROP_BOOL(arg0, "owner"),
    PROP_BOOL(arg0, "threads"),
    PROP_BOOL(arg0, "priority")
  };

  auto *callback = new Nan::Callback(info[1].As<Function>());

  Nan::AsyncQueueWorker(new SnapshotWorker(callback, fields));
}
