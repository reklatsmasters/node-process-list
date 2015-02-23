# node-process-list

> Cross-platform native method to receive the list of the launched processes.

## Install
You need build tools for native module.

1. Install Python 2.7
2. _Windows_: Install MSVC 2013 or newer
3. _Unix_: install GCC to 4.8 or newer
4. install node build tool `npm i -g pangyp` See [pangyp](https://npmjs.org/package/pangyp) for more info.

```bash
npm i process-list
```
Supported OS:
* Windows 7+ (maybe Vista and XP)
* Linux (Ubuntu, CentOS and other)
* MacOS - ???.

Supported engines:
* nodejs 0.10+
* iojs

**N.B.** Windows x64 + iojs <= 1.3 has error when build.

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
// 		ppid: 12,                   // parent ptocess pid
//	    priority: 15                // process priority (OS specific value)
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
* Сustomizable information
* Dependencies of the process
* ~~Process priority~~
