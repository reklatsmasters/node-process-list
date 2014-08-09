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
	exports->Set(String::NewSymbol("snapshot"),
		FunctionTemplate::New(snapshot)->GetFunction());

	// alias
	exports->Set(String::NewSymbol("list"),
		FunctionTemplate::New(snapshot)->GetFunction());
}

NODE_MODULE(tasklist, init)