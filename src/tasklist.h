/**
 * Copyright (c) 2014 Dmitry Tsvettsikh <https://github.com/reklatsmasters>
 *
 * MIT License <https://github.com/reklatsmasters/node-process-list/blob/master/LICENSE>
 */

#ifndef SRC_TASKLIST_H_
#define SRC_TASKLIST_H_

#include <stdint.h>
#include <vector>
#include <memory>
#include <string>

#define NORMAL(x, low, high) (((x) > (high))?(high):(((x) < (low))?(low):(x)))

namespace pl {

struct process {
  uint32_t pid = 0;
  uint32_t ppid = 0;

  std::string path;
  std::string name;
  std::string cmdline;

  std::string owner;

  uint32_t threads = 0;
  int32_t priority = 0;

  uint64_t starttime = 0;
  uint64_t vmem = 0;
  uint64_t pmem = 0;

  double cpu = 0;
  uint64_t utime = 0;
  uint64_t stime = 0;
};

struct process_fields {
  bool pid;
  bool ppid;

  bool path;
  bool name;
  bool owner;
  bool cmdline;

  bool threads;
  bool priority;

  bool starttime;
  bool vmem;
  bool pmem;

  bool cpu;
  bool utime;
  bool stime;
};

typedef std::vector<process> list_t;

list_t list(const struct process_fields &);

};  // namespace pl

#endif  // SRC_TASKLIST_H_
