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

#pragma GCC diagnostic ignored "-Wunused-result";

struct procstat_t {
  uint32_t pid;
  uint32_t ppid;
  uint32_t threads;
  int32_t  priority;
  uint64_t uptime;
  
  uint64_t utime;   // CPU time spent in user code (in sec)
  uint64_t stime;   // CPU time spent in kernel code (in sec)
};

static int hertz = 0;

static inline uint64_t adjust_time(uint64_t t) {
   if (hertz == 0) {
     hertz = sysconf(_SC_CLK_TCK);
   }

   return t / hertz;
}

static inline bool
is_pid(const char *data, size_t s)
{
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

static void procstat(const char *fpid, procstat_t *pstat) {
  char path[32];
  snprintf(path, sizeof(path), "/proc/%s/stat", fpid);

  auto fd = fopen(path, "r");

  if (fd == NULL) {
    throw std::runtime_error("can't open stat");
  }

  fscanf(fd, "%d", &pstat->pid);     // (1)
  fscanf(fd, " %*s");
  fscanf(fd, " %*c");
  fscanf(fd, " %d", &pstat->ppid);    // (4)
  fscanf(fd, " %*d");
  fscanf(fd, " %*d");
  fscanf(fd, " %*d");
  fscanf(fd, " %*d");
  fscanf(fd, " %*u");
  fscanf(fd, " %*u");
  fscanf(fd, " %*u");
  fscanf(fd, " %*u");
  fscanf(fd, " %*u");
  fscanf(fd, " %lu", &pstat->utime);     // (14)
  fscanf(fd, " %lu", &pstat->stime);     // (15)
  fscanf(fd, " %*d");
  fscanf(fd, " %*d");
  fscanf(fd, " %d", &pstat->priority);  // (18)
  fscanf(fd, " %*d");
  fscanf(fd, " %d", &pstat->threads);   // (20)
  fscanf(fd, " %*d");
  fscanf(fd, " %lu", &pstat->uptime); // (22)

  fclose(fd);

  pstat->utime = adjust_time(pstat->utime);
  pstat->stime = adjust_time(pstat->stime);
  pstat->uptime = adjust_time(pstat->uptime);
}

namespace pl {

  list_t list(const struct process_fields &requested_fields) {
    list_t proclist;
    struct sysinfo sys_info;

    if (sysinfo(&sys_info) != 0) {
      throw new std::logic_error("`sysinfo` return non-zero code");
    }

    auto dirlist = ls("/proc", [](const struct dirent *entry) {
      return is_pid(entry->d_name, strlen(entry->d_name));
    });

    for (auto entry : dirlist) {
      struct process proc;

      struct procstat_t pstat;
      procstat(entry.d_name, &pstat);

      struct timeval tv;
      gettimeofday(&tv, NULL);

      uint64_t now = tv.tv_sec * 1000L + tv.tv_usec / 1000L;

      if (requested_fields.cmdline) {
        proc.cmdline = cmdline(entry.d_name);
      }

      if (requested_fields.owner) {
        proc.owner = owner(entry.d_name);
      }

      if (requested_fields.path || requested_fields.name) {
        procpath(entry.d_name, &proc);
      }

      if (requested_fields.pid) {
        proc.pid = pstat.pid;
      }

      if (requested_fields.ppid) {
        proc.ppid = pstat.ppid;
      }

      if (requested_fields.threads) {
        proc.threads = pstat.threads;
      }

      if (requested_fields.priority) {
        proc.priority = pstat.priority;
      }

      if (requested_fields.starttime) {
        proc.starttime = now - (sys_info.uptime * 1000L - pstat.uptime * 1000L);
      }

      // @link http://stackoverflow.com/a/16736599/1556249
      if (requested_fields.cpu) {
        double elapsed = sys_info.uptime - pstat.uptime;
        proc.cpu = (pstat.utime + pstat.stime) / elapsed * 100.0;
      }

      proclist.push_back(proc);
    }

    return proclist;
  }
}  // namespace pl
