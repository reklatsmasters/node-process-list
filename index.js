'use strict'

const ps = require('./lib/processlist')
const then = require('pify')

const es6snapshot = then(ps.snapshot)

module.exports = {
  snapshot
}

/**
 * get process list
 * @param {Object} opts
 * @param {bool} opts.verbose
 */
function snapshot(opts) {
  opts = opts || {}

  return es6snapshot(opts.verbose)
}