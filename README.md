<p align='center'>
  <img src='logo.png' height='250' width='483' alt='logo' />
  <p align='center'>Cross-platform native method to receive the list of the launched processes</p>
</p>

[![Build Status](https://travis-ci.org/reklatsmasters/node-process-list.svg?branch=master)](https://travis-ci.org/reklatsmasters/node-process-list) [![Build status](https://ci.appveyor.com/api/projects/status/oy0sbnie2a0d5hou?svg=true)](https://ci.appveyor.com/project/ReklatsMasters/node-process-list) [![npm](https://img.shields.io/npm/v/process-list.svg)](https://npmjs.org/package/process-list) [![license](https://img.shields.io/npm/l/process-list.svg)](https://npmjs.org/package/process-list) [![downloads](https://img.shields.io/npm/dm/process-list.svg)](https://npmjs.org/package/process-list) 
[![Greenkeeper badge](https://badges.greenkeeper.io/reklatsmasters/node-process-list.svg)](https://greenkeeper.io/)

### Install

```bash
npm i process-list
# or
yarn add process-list
```

It's that easy! npm will download one of the [prebuilt](https://github.com/reklatsmasters/node-process-list/releases/latest) binaries for your OS. If you need to build `process-list`, see [node-gyp](https://npmjs.org/package/node-gyp) for more details.

### Supported OS

* `Windows` Windows 7+, Windows Server 2008 R2+
* `Linux` any Linux-based distributives
* `OS X` *Soon...*

### Usage
```js
const { snapshot } = require("process-list");

const tasks = await snapshot('pid', 'name');
console.log(tasks);

// output
// [{
//    name: "1.exe",
//    pid: 1234,
// }, ... ]
```

### API

##### `snapshot(...field: String): Promise<[]Object>`
Returns the list of the launched processes.

##### `allowedFields: []String`
List of allowed fields.

* `pid: Number` - process pid
* `ppid: Number` - parent process pid
* `name: String` - process name (title)
* `path: String` - full path to the process binary file
* `threads: Number` - threads per process
* `owner: String` - the owner of the process
* `priority: Number` - an os-specific process priority
* `cmdline: String` - full command line of the process
* `starttime: Date` - the process start date / time
* `vmem: String` - virtual memory size in bytes used by process
* `pmem: String` - physical memory size in bytes used by process
* `cpu: Number` - cpu usage by process in percent
* `utime: String` - amount of time in ms that this process has been scheduled in user mode
* `stime: String` - amount of time that in ms this process has been scheduled in kernel mode

## License

MIT, Copyright &copy; 2014 - 2019 Dmitry Tsvettsikh
