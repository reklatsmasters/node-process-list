#ifndef V8DECODE_H
#define V8DECODE_H

#include <node.h>

v8::Handle<v8::Value> snapshot_sync(const v8::Arguments& args);
v8::Handle<v8::Value> snapshot_async(const v8::Arguments& args);

#endif