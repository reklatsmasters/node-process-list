#include <nan.h>
#include "snapshot.h"  // NOLINT(build/include)

NAN_MODULE_INIT(init) {
  Nan::Export(target, "snapshot", snapshot);
}

NODE_MODULE(processlist, init);
