#ifndef SRC_TASKLIST_H_
#define SRC_TASKLIST_H_

#include <stdint.h>
#include <vector>
#include <memory>
#include <string>

namespace pl {

class Process {
 public:
  ~Process() {}

  virtual std::string name() const = 0;
  virtual uint32_t pid() const = 0;
  virtual uint32_t parentPid() const = 0;
  virtual std::string path() const = 0;
  virtual uint32_t threads() const = 0;
  virtual std::string owner() const = 0;
  virtual int32_t priority() const = 0;
};

namespace task {
  typedef std::vector<std::shared_ptr<Process>> list_t;

  list_t list();
};  // task

};  // namespace pl

#endif  // SRC_TASKLIST_H_
