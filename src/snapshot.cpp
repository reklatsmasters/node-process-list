#ifndef BUILDING_NODE_EXTENSION
	#define BUILDING_NODE_EXTENSION
#endif

#include <node.h>
#include "snapshot.h"
#include "tasklist.h"

#include <algorithm>
#include <memory>

using namespace v8;

Handle<Value> snapshot(const Arguments& args) {
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
			          String::New("path"),
			          String::New( snap.at(i)->path().c_str() )
			);

			tasks->Set(i, hash);
		} else {
			tasks->Set(i, String::New( snap.at(i)->name().c_str() ));
		}
	}

	return scope.Close(tasks);
}