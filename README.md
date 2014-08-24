# node-process-list

> Cross-platform method to receive the list of the launched processes.

## Install
You do not need a compiler to compile the package. Module is already compiled. Just install.

```bash
npm install process-list
```
Supported OS:
* Windows 7+ (maybe Vista)
* Linux (Ubuntu, CentOS and other)
* MacOS - supported, but **not tested**.

#### install from sources
If none of the compiled binaries does not fit, you will be asked to compile the package from source.

1. Install Python 2.7
2. For Windows: Install MSVC 2013
3. For Unix: install / update GCC to 4.8 and later
4. ``` npm install -g node-gyp ```
5. ``` npm install process-list ```
    
See [node-gyp](https://github.com/TooTallNate/node-gyp) for more info.


### Example: simple info
```js
var tasks = require("process-list");

tasks.snapshot(function(err, data){
	console.log(data);
});

// output
// ["1.exe", "2.exe", ... ]
```

### Example 2: Extended info
```js
var tasks = require("process-list");

tasks.snapshot({verbose:true}, function(err, data){
	console.log(data);
});

// output
// [{
// 		name: "1.exe",              // process name
// 		path: "c:\\windows\\1.exe", // full path to process (if available)
// 		threads: 5, 
// 		owner:"root",
// 		pid: 1234,                  // process pid
// 		ppid: 12	                // parent ptocess pid
// }, ... ]
```

### Example 3: Sync API
```js
var tasks = require("process-list");
console.log(tasks.snapshotSync());
```

## TODO
* CPU usage per process
* Memory usage per process
* Command line (for Windows)
* Process priority
* Dependencies of the process
* Сustomizable information
