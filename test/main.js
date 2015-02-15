'use strict';

/* global describe, it */

var assert = require('assert');
var tasklist = require('../');

describe("module #main", function(){

  describe("method #snapshot", function() {
    it("should work", function(done) {
      tasklist.snapshot(function(err, data){
        assert.ok(Array.isArray(data));
        assert.ok(data.length);

        done();
      });
    });

    it("should work with verbose", function(done) {
      tasklist.snapshot({verbose: true}, function(err, data){
        assert.ok(Array.isArray(data));
        assert.ok(data.length);

        done();
      });
    });
  });

  describe("method #snapshotSync", function() {
    it("should work", function() {
      var tasks = tasklist.snapshotSync();

      assert.ok(Array.isArray(tasks));
      assert.ok(tasks.length);
    });

    it("should work with verbose", function() {
      var tasks = tasklist.snapshotSync();

      assert.ok(Array.isArray(tasks));
      assert.ok(tasks.length);
    });
  });
});
