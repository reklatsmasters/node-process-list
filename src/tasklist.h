#ifndef SRC_TASKLIST_H_
#define SRC_TASKLIST_H_

#include <stdint.h>
#include <vector>
#include <memory>
#include <string>

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
};

struct process_fields {
  bool pid;
  bool ppid;

  bool path;
  bool name;
  bool owner;

  bool threads;
  bool priority;
};

typedef std::vector<process> list_t;

list_t list(const struct process_fields &);

};  // namespace pl

#endif  // SRC_TASKLIST_H_
