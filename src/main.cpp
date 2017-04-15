/**
 * Copyright (c) 2014 Dmitry Tsvettsikh <https://github.com/reklatsmasters>
 *
 * MIT License <https://github.com/reklatsmasters/node-process-list/blob/master/LICENSE>
 */

#include <nan.h>
#include "snapshot.h"  // NOLINT(build/include)

NAN_MODULE_INIT(init) {
  Nan::Export(target, "snapshot", snapshot);
}

NODE_MODULE(processlist, init);
