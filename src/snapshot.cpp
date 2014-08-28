#ifndef BUILDING_NODE_EXTENSION
	#define BUILDING_NODE_EXTENSION
#endif

#include <node.h>
#include "snapshot.h"
#include "tasklist.h"

#include <algorithm>
#include <memory>

using namespace v8;

Handle<Value> snapshot_sync(const Arguments& args) {
	HandleScope scope;
	bool verbose = false;

	// get list running processes
	// TODO: exceptions
	std::vector< std::shared_ptr<Process> > snap = tasklist ();
	Local<Array> tasks = Array::New(snap.size());

	// read options
	if (args.Length() && args[0]->IsObject()) {
		Local<Object> options = args[0]->ToObject();

		if (options->Has(String::New("verbose"))) {
			verbose = options->Get(String::New("verbose"))->ToBoolean()->Value();
		}
	}

	for (uint32_t i = 0; i < tasks->Length(); ++i) {
		if (verbose) {
			Local<Object> hash = Object::New();

			hash->Set(
			          String::New("name"),
			          String::New( snap.at(i)->name().c_str() )
			);

			hash->Set(
			          String::New("pid"),
			          Number::New( snap.at(i)->pid() )
			);

			hash->Set(
			          String::New("ppid"),
			          Number::New( snap.at(i)->parentPid() )
			);

			hash->Set(
			          String::New("path"),
			          String::New( snap.at(i)->path().c_str() )
			);

			hash->Set(
			          String::New("threads"),
			          Number::New( snap.at(i)->threads() )
			);

			hash->Set(
			          String::New("owner"),
			          String::New( snap.at(i)->owner().c_str() )
			);

			hash->Set(
			          String::New("priority"),
			          Number::New( snap.at(i)->priority() )
			);

			tasks->Set(i, hash);
		} else {
			tasks->Set(i, String::New( snap.at(i)->name().c_str() ));
		}
	}

	return scope.Close(tasks);
}

typedef struct _AsyncData {
	Persistent<Function>                    callback;
	std::vector< std::shared_ptr<Process> > tasks;
	bool                                    verbose;
} AsyncData;

void AsyncWork(uv_work_t *job) {
	AsyncData *asyncData = (AsyncData *)job->data;
	asyncData->tasks = tasklist ();
}

void AsyncAfter(uv_work_t *job) {
	HandleScope scope;

	AsyncData *asyncData = (AsyncData *)job->data;
	Local<Array> tasks = Array::New(asyncData->tasks.size());

	for (uint32_t i = 0; i < tasks->Length(); ++i) {
		if (asyncData->verbose) {
			Local<Object> hash = Object::New();

			hash->Set(
			          String::New("name"),
			          String::New( asyncData->tasks.at(i)->name().c_str() )
			);

			hash->Set(
			          String::New("pid"),
			          Number::New( asyncData->tasks.at(i)->pid() )
			);

			hash->Set(
			          String::New("ppid"),
			          Number::New( asyncData->tasks.at(i)->parentPid() )
			);

			hash->Set(
			          String::New("path"),
			          String::New( asyncData->tasks.at(i)->path().c_str() )
			);

			hash->Set(
			          String::New("threads"),
			          Number::New( asyncData->tasks.at(i)->threads() )
			);

			hash->Set(
			          String::New("owner"),
			          String::New( asyncData->tasks.at(i)->owner().c_str() )
			);


			hash->Set(
			          String::New("priority"),
			          Number::New( asyncData->tasks.at(i)->priority() )
			);

			tasks->Set(i, hash);
		} else {
			tasks->Set(i, String::New( asyncData->tasks.at(i)->name().c_str() ));
		}
	}

	Handle<Value> argv[] = {
	    Undefined(),
	    tasks
	};

	TryCatch try_catch;
	asyncData->callback->Call(Context::GetCurrent()->Global(), 2, argv);

	if (try_catch.HasCaught()) {
		node::FatalException(try_catch);
	}

	asyncData->callback.Dispose();
	delete asyncData;
	delete job;
}

v8::Handle<v8::Value> snapshot_async(const v8::Arguments& args) {
	HandleScope scope;

	bool verbose = false;
	Persistent<Function> cb;

	// read options
	if ( args.Length() ) {

		if (args[0]->IsObject()) {
			Local<Object> options = args[0]->ToObject();

			if (options->Has(String::New("verbose"))) {
				verbose = options->Get(String::New("verbose"))->ToBoolean()->Value();
			}
		}

		// callback required
		if ( (args.Length() == 1) && args[0]->IsFunction() ) {
			cb = Persistent<Function>::New( Local<Function>::Cast(args[0]) );
		} else if ( (args.Length() >= 2) && args[1]->IsFunction()) {
			cb = Persistent<Function>::New( Local<Function>::Cast(args[1]) );
		} else {
			ThrowException(Exception::TypeError(String::New("Wrong arguments: callback function required")));
			return scope.Close(Undefined());
		}
	} else {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments: 1 argument required, but only 0 present")));
		return scope.Close(Undefined());
	}

	uv_work_t *job = new uv_work_t;
	AsyncData *asyncData = new AsyncData;

	asyncData->verbose = verbose;
	asyncData->callback = cb;

	job->data = asyncData;

	uv_queue_work(
	              uv_default_loop(),
	              job,
	              AsyncWork,
	              (uv_after_work_cb)AsyncAfter
	);

	return scope.Close(Undefined());
}