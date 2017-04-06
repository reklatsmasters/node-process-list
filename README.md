# process-list [![Build Status](https://travis-ci.org/reklatsmasters/node-process-list.svg?branch=master)](https://travis-ci.org/reklatsmasters/node-process-list) [![npm](https://img.shields.io/npm/v/process-list.svg)](https://npmjs.org/package/process-list) [![license](https://img.shields.io/npm/l/process-list.svg)](https://npmjs.org/package/process-list) [![downloads](https://img.shields.io/npm/dm/process-list.svg)](https://npmjs.org/package/process-list) [![JavaScript Style Guide](https://img.shields.io/badge/code_style-standard-brightgreen.svg)](https://standardjs.com)

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
```js
const { snapshot } = require("process-list");

snapshot().then(tasks => console.log(tasks))
// snapshot('pid', 'name', 'path')

// output
// [{
//    name: "1.exe",
// 	  path: "c:\\windows\\1.exe",
//    pid: 1234,
// }, ... ]
```

### API

##### `snapshot(...field: String): Promise<[]Object>`
Returns a list of runned processes.

##### `allowedFields: []String`
A list of allowed fields

* `pid` - process pid
* `ppid` - parent process pid
* `name` - process name (title)
* `path` - full path to the process binary file
* `threads` - threads per process
* `owner` - the owner of the process
* `priority` - an os-specific process priority

## License

MIT, 2014 (c) Dmitry Tsvettsikh
