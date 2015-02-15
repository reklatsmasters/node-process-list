#include <node.h>
#include <nan.h>
#include "snapshot.h"

using namespace v8;

void init(Handle<Object> exports) {
	// sync api
	exports->Set(NanNew<String>("snapshotSync"),
		NanNew<FunctionTemplate>(snapshot_sync)->GetFunction());

	// async api
	exports->Set(NanNew<String>("snapshot"),
		NanNew<FunctionTemplate>(snapshot_async)->GetFunction());
}

NODE_MODULE(process_list, init)
