/**
 * Copyright (c) 2014 Dmitry Tsvettsikh <https://github.com/reklatsmasters>
 *
 * MIT License <https://github.com/reklatsmasters/node-process-list/blob/master/LICENSE>
 */

#include "tasklist.h"  // NOLINT(build/include)

#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/types.h>  // ssize_t
#include <sys/time.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>  // read, basename
#include <fcntl.h>
#include <pwd.h>
#include <libgen.h>  // readlink
#include <stdio.h>

#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

using pl::process;

static inline bool is_pid(const char *data, size_t s) {
  return std::all_of (data, data + s, ::isdigit);
}

template<class FilterPredicate>
static std::vector<dirent> ls(const char *path, FilterPredicate filter) {
  auto dir = opendir(path);
  struct dirent *entry;
  std::vector<dirent> dirlist;

  if (!dir) {
    perror("can't read dir");
    throw std::bad_alloc();
  }

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
      continue;
    }

    if (filter(entry)) {
      dirlist.push_back(*entry);  // copy dirent
    }
  }

  closedir(dir);
  return dirlist;
}

/**
 * from htop
 * @link https://github.com/hishamhm/htop
 */
static ssize_t xread(int fd, void *buf, size_t count) {
  // Read some bytes. Retry on EINTR and when we don't get
  // as many bytes as we requested.
  size_t alreadyRead = 0;

  for (;;) {
    ssize_t res = read(fd, buf, count);

    if (res == -1 && errno == EINTR) {
      continue;
    }

    if (res > 0) {
      buf = reinterpret_cast<char *>(buf) + res;
      count -= res;
      alreadyRead += res;
    }

    if (res == -1) {
      alreadyRead = -1;
      break;
    }

    if (count == 0 || res == 0) {
      break;
    }
  }

  return alreadyRead;
}

static std::string cmdline(const char* pid) {
  char path[32];
  snprintf(path, sizeof(path), "/proc/%s/cmdline", pid);

  int fd = open(path, O_RDONLY);

  if (fd == -1) {
    throw std::runtime_error("can't open cmdline");
  }

  const int MAX_READ = 4096;
  char command[MAX_READ + 1];
  int amtRead = xread(fd, command, MAX_READ);

  close(fd);

  if (amtRead > 0) {
    for (int i = 0; i < amtRead; ++i) {
      if (command[i] == '\0' || command[i] == '\n') {
        command[i] = ' ';
      }
    }
  }

  return std::string(command, amtRead - 1);
}

static std::string owner(const char *pid) {
  struct stat sstat;
  struct passwd usrpwd, *res;
  char buf[1024];
  int bufsize = sizeof(buf);
  std::string username;

  char path[32];
  snprintf(path, sizeof(path), "/proc/%s", pid);

  if (stat(path, &sstat) == -1) {
    throw std::runtime_error("can't stat dir");
  }

  getpwuid_r(sstat.st_uid, &usrpwd, buf, bufsize, &res);

  if (res != NULL) {
    username = usrpwd.pw_name;
  }

  return username;
}

static void procpath(const char *pid, process *proc) {
  char syspath[32];
  snprintf(syspath, sizeof(syspath), "/proc/%s/exe", pid);

  char path[4096+1];
  ssize_t size = readlink(syspath, path, sizeof(path) - 1);

  if (size == -1) {
    return;
  }

  path[size] = '\0';

  proc->path = std::string(path);
  proc->name = std::string(basename(path));
}

static void procstat(const char *fpid, process *proc) {
  char path[32];
  snprintf(path, sizeof(path), "/proc/%s/stat", fpid);

  auto fd = fopen(path, "r");

  if (fd == NULL) {
    throw std::runtime_error("can't open stat");
  }

  fscanf(fd, "%d", &proc->pid);     // (1)
  fscanf(fd, "%*s");
  fscanf(fd, "%*c");
  fscanf(fd, "%d", &proc->ppid);    // (4)
  fscanf(fd, "%*d");
  fscanf(fd, "%*d");
  fscanf(fd, "%*d");
  fscanf(fd, "%*d");
  fscanf(fd, "%*u");
  fscanf(fd, "%*u");
  fscanf(fd, "%*u");
  fscanf(fd, "%*u");
  fscanf(fd, "%*u");
  fscanf(fd, "%*u");
  fscanf(fd, "%*u");
  fscanf(fd, "%*d");
  fscanf(fd, "%*d");
  fscanf(fd, "%d", &proc->priority);  // (18)
  fscanf(fd, "%*d");
  fscanf(fd, "%d", &proc->threads);
  fscanf(fd, "%*d");
  fscanf(fd, "%lu", &proc->starttime);

  fclose(fd);
}

namespace pl {

  list_t list(const struct process_fields &requested_fields) {
    list_t proclist;
    struct sysinfo sys_info;

    int sys_ret = sysinfo(&sys_info);
    uint64_t jiffies_per_second = sysconf(_SC_CLK_TCK);

    auto dirlist = ls("/proc", [](const struct dirent *entry) {
      return is_pid(entry->d_name, strlen(entry->d_name));
    });

    for (auto entry : dirlist) {
      struct process proc;

      if (requested_fields.cmdline) {
        proc.cmdline = cmdline(entry.d_name);
      }

      if (requested_fields.owner) {
        proc.owner = owner(entry.d_name);
      }

      if (requested_fields.path || requested_fields.name) {
        procpath(entry.d_name, &proc);
      }

      if (
        requested_fields.pid ||
        requested_fields.ppid ||
        requested_fields.threads ||
        requested_fields.priority) {
        procstat(entry.d_name, &proc);
      }

      if (requested_fields.starttime) {
        if (sys_ret != 0) {
          proc.starttime = 0;
        } else {
          struct timeval tv;
          gettimeofday(&tv, NULL);

          uint64_t now = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
          auto start_since_boot = proc.starttime / jiffies_per_second;

          proc.starttime = now - (sys_info.uptime - start_since_boot);
        }
      }

      proclist.push_back(proc);
    }

    return proclist;
  }
}  // namespace pl
