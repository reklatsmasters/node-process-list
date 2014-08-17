# node-process-list

> Cross-platform method to receive the list of the launched processes.

## Install
* from bonaries

```bash
npm install process-list
```
* from sources
	1. Install Python 2.7
    2. For Windows: Install MSVC 2013
    3. For linux: install / update GCC to 4.8 and later
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
// 		name: "1.exe", 
// 		path: "c:\\windows\\1.exe", 
// 		threads: 5, 
// 		owner:"Ololo",
// 		pid: 1234,
// 		ppid: 12
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
