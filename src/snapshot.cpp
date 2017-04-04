#include <nan.h>
#include "snapshot.h"
#include "tasklist.h"

#include <algorithm>
#include <memory>

using namespace v8;

#define _NAN_STRING(s) Nan::New<v8::String>(s).ToLocalChecked()

class SnapshotWorker : public Nan::AsyncWorker {
public:
	SnapshotWorker(Nan::Callback *callback, bool _verbose = false)
	: Nan::AsyncWorker(callback), verbose(_verbose)
	{
	}

	~SnapshotWorker(){};

	void Execute () {
		tasks = tasklist ();
	}

	void HandleOKCallback () {
		Nan::HandleScope scope;

		Local<Array> jobs = Nan::New<Array>( tasks.size() );

		for (uint32_t i = 0; i < jobs->Length(); ++i) {
			if (verbose) {
				Local<Object> hash = Nan::New<Object>();

				Nan::Set(hash, _NAN_STRING("name"), _NAN_STRING(tasks.at(i)->name()));
				Nan::Set(hash, _NAN_STRING("pid"), Nan::New<Number>(tasks.at(i)->pid()));
				Nan::Set(hash, _NAN_STRING("ppid"), Nan::New<Number>(tasks.at(i)->parentPid()));
				Nan::Set(hash, _NAN_STRING("path"), _NAN_STRING(tasks.at(i)->path()));
				Nan::Set(hash, _NAN_STRING("threads"), Nan::New<Number>(tasks.at(i)->threads()));
				Nan::Set(hash, _NAN_STRING("owner"), _NAN_STRING(tasks.at(i)->owner()));
				Nan::Set(hash, _NAN_STRING("priority"), Nan::New<Number>(tasks.at(i)->priority()));

				Nan::Set(jobs, i, hash);
			} else {
				Nan::Set(jobs, i, _NAN_STRING(tasks.at(i)->name()));
			}
		}

		Local<Value> argv[] = {
			Nan::Null(),
			jobs
		};

		callback->Call(2, argv);
	}

	void HandleErrorCallback () {
		Nan::HandleScope scope;

		Local<Value> argv[] = {
			Nan::Error("internal error")
		};

		callback->Call(1, argv);
	}

private:
	tasklist_t tasks;
	bool verbose;
};

NAN_METHOD(snapshot) {
	bool verbose = Nan::To<bool>(info[0]).FromJust();
	auto *callback = new Nan::Callback(info[1].As<Function>());

	Nan::AsyncQueueWorker(new SnapshotWorker(callback, verbose));
}
