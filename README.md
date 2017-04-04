# process-list [![npm](https://img.shields.io/npm/v/process-list.svg)](https://npmjs.org/package/process-list) [![license](https://img.shields.io/npm/l/process-list.svg)](https://npmjs.org/package/process-list) [![downloads](https://img.shields.io/npm/dm/process-list.svg)](https://npmjs.org/package/process-list)

Cross-platform native method to receive the list of the launched processes.

## Install
You need build tools for native module.

1. Install Python 2.7
2. _Windows_: Install MSVC 2013 or newer
3. _Unix_: install GCC to 4.8 or newer
4. install node build tool `npm i -g node-gyp` See [node-gyp](https://npmjs.org/package/node-gyp) for more details.

```bash
npm i process-list
```


### Usage
####Example 1: Simple info
```js
const { snapshot } = require("process-list");

snapshot().then(tasks => console.log(tasks))

// output
// ["1.exe", "2.exe", ... ]
```

#### Example 2: Extended info
```js
const { snapshot } = require("process-list");

snapshot().then(tasks => console.log(tasks))

// output
// [{
// 	  name: "1.exe",              // process name
// 	  path: "c:\\windows\\1.exe", // full path to the process (if available)
// 	  threads: 5, 
// 	  owner:"root",
//    pid: 1234,                  // process pid
//    ppid: 12,                   // parent ptocess pid
//    priority: 15              // process priority (OS specific value)
// }, ... ]
```

## License

MIT, 2014 (c) Dmitry Tsvettsikh
