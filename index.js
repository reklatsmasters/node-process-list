'use strict';

var scope = require('./lib/process-list');

// sync api
exports.snapshotSync = function(opts) {
  return scope.snapshotSync(opts.verbose);
};

// async api
exports.snapshot = function(opts, cb) {
  if (typeof opts === 'function') {
    cb = opts;
    opts = {};
  }

  return scope.snapshot(opts.verbose, cb);
};
