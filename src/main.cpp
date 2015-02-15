#include <nan.h>
#include "snapshot.h"

void init(v8::Handle<v8::Object> exports) {
	NODE_SET_METHOD(exports, "snapshotSync", snapshot_sync);
	NODE_SET_METHOD(exports, "snapshot", snapshot_async);
}

NODE_MODULE(processlist, init);
