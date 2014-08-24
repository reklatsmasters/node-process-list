'use strict';

var find = require('find-file');
var path = require('path');
var url = require('url');
var logSymbols = require('log-symbols');
var pkg = require('./package.json');
var spawn  = require("child_process").spawn;

require('string.prototype.endswith');

var moduleCheck = function(rpath, cb, thisArg) {
	try {
		require(rpath);
	} catch(e) {
		cb.call(thisArg, e, false);
		return;
	}

	cb.call(thisArg, null, true);
};

var moduleBuilder = function(cb) {
	var nodeGypPath = find("node-gyp");

	if (!nodeGypPath) {
		return cb(new Error("Couldn\'t find the `node-gyp` binary. Make sure it\'s installed and in your $PATH."));
	}

	var cmd = process.platform === "win32" ? nodeGypPath[0] + ".cmd" : nodeGypPath[0];
	var nodeGyp = spawn(cmd, ['rebuild'], {stdio: [ 'ignore', process.stdout, process.stderr ]});

	nodeGyp.on("error", function(err){
		console.error(logSymbols.error, err.message);
	});

	nodeGyp.on("close", function(code){
		cb(code ? new Error("Build failed!") : null);
	});
};

/**
 * Initialize a new `BinWrapper`
 *
 * @api public
 */

function BinWrapper() {
    if (!(this instanceof BinWrapper)) {
        return new BinWrapper();
    }

    this._src = [];
}

/**
 * Add a file to download
 *
 * @param {String} src
 * @param {String} os
 * @param {String} arch
 * @api public
 */

BinWrapper.prototype.src = function (src, os, arch) {
    if (!arguments.length) {
        return this._src;
    }

    var obj = { url: src, name: path.basename(src) };

    if (os) {
        obj.os = os;
    }

    if (arch) {
        obj.arch = arch;
    }

    this._src = this._src.concat(obj);
    return this;
};

/**
 * Define where to download the file to
 *
 * @param {String} dest
 * @api public
 */

BinWrapper.prototype.dest = function (dest) {
    if (!arguments.length) {
        return this._dest;
    }

    this._dest = dest;
    return this;
};

/**
 * Define which file to use as a binary
 *
 * @param {String} bin
 * @api public
 */

BinWrapper.prototype.use = function (bin) {
    if (!arguments.length) {
        return this._use;
    }

    if (!bin.endsWith(".node")) {
    	bin += ".node";
    }

    this._use = bin;
    return this;
};

/**
 * Get the bin path
 *
 * @api public
 */

BinWrapper.prototype.path = function () {
    var opts = { path: this.dest(), global: this.global, exclude: 'node_modules/.bin' };
    var bin = find(this.use(), opts);
    var dir;

    if (bin && bin.length > 0) {
        dir = bin[0];
    } else {
        dir = path.join(this.dest(), this.use());
    }

    return dir;
};

/**
 * Run bin-wrapper
 *
 * @param {Array} cmd
 * @param {Function} cb
 * @api public
 */

BinWrapper.prototype.install = function (cb, thisArg) {
    var Download = require('download');
    var files = this._parse(this.src());

    this._test(function (err) {
        if (err) {
            var download = new Download({ mode: 777 });

            files.forEach(function (file) {
                download.get(file, this.dest());
            }, this);

            return download.run(function (err) {
                if (err) {
                    cb.call(thisArg, err);
                    return;
                }

                this._test(function (err) {
                    if (err) {
                        cb.call(thisArg, err);
                        return;
                    }

                    cb.call(thisArg);
                });
            }.bind(this));
        }

        cb.call(thisArg);
    },  this);
};

BinWrapper.prototype._require = function() {
	var $path = this.path();

	if ($path.endsWith(".node")) {
		$path = $path.substring(0, $path.length - 5);
	}

	return $path;
}

/**
 * Test binary
 *
 * @param {Array} cmd
 * @param {Function} cb
 * @api private
 */

BinWrapper.prototype._test = function (cb, thisArg) {
    if (this.path()) {
        return moduleCheck(this._require(), function (err, works) {
            if (err) {
                cb.call(thisArg, err);
                return;
            }

            if (!works) {
                cb.call(thisArg, new Error('The `' + this.use() + '` binary doesn\'t seem to work correctly.'));
                return;
            }

            cb.call(thisArg);
        }.bind(this));
    }

    cb.call(thisArg, new Error('Couldn\'t find the `' + this.use() + '` binary. Make sure it\'s installed and in your $PATH.'));
};

/**
 * Parse
 *
 * @param {Object} obj
 * @api private
 */

BinWrapper.prototype._parse = function (obj) {
    var arch = process.arch === 'x64' ? 'x64' : process.arch === 'arm' ? 'arm' : 'x86';
    var ret = [];

    obj.filter(function (o) {
        if (o.os && o.os === process.platform && o.arch && o.arch === arch) {
            return ret.push(o);
        } else if (o.os && o.os === process.platform && !o.arch) {
            return ret.push(o);
        } else if (!o.os && !o.arch) {
            return ret.push(o);
        }
    });

    return ret;
};

var BASE_URL = 'https://raw.githubusercontent.com/reklatsmasters/node-process-list/master/dist/';

var bin = new BinWrapper()
        .src(url.resolve(BASE_URL, "linux/x86/" + pkg.name + ".node"), 'linux')
        .src(url.resolve(BASE_URL, "win/x86/" + pkg.name + ".node"), 'win32')
        .dest(path.join(__dirname, './lib'))
        .use(pkg.name + ".node")
;

bin.install(function(err) {
	if (err) {
		console.warn(logSymbols.warning , "pre-build test failed, compiling from source...");

		return moduleBuilder(function(err){
			if (err) {
				console.error(logSymbols.error, err.message);
				return;
			}

			console.log(logSymbols.success, pkg.name,'built successfully!');
		});
	}

	console.log(logSymbols.success, "pre-build test passed successfully!");
});