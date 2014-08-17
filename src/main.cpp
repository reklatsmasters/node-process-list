#ifdef _MSC_VER
	#pragma warning(disable:4506)
	#pragma warning(disable:4005)
#endif

#ifndef BUILDING_NODE_EXTENSION
	#define BUILDING_NODE_EXTENSION
#endif

#include <node.h>
#include "snapshot.h"

using namespace v8;

void init(Handle<Object> exports) {
	// sync api
	exports->Set(String::NewSymbol("snapshotSync"),
		FunctionTemplate::New(snapshot_sync)->GetFunction());

	// async api
	exports->Set(String::NewSymbol("snapshot"),
		FunctionTemplate::New(snapshot_async)->GetFunction());
}

NODE_MODULE(process_list, init)