#include <iostream>
#include <iomanip>
#include <memory>
#include <stdint.h>
#include <algorithm>

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>  // ssize_t
#include <unistd.h>     // read, basename
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <cstring>
#include <libgen.h>	// readlink

#include "tasklist.h"

#ifndef MAX_READ
	#define MAX_READ 2048
#endif

// check if string is int
bool is_pid (const std::string &data) {
	if (data.empty ()) {
		return false;
	}

	return std::any_of (std::begin (data), std::end (data), [](const char &c) {
		return std::isdigit(c);
	});
}

//from htop @link( https://github.com/hishamhm/htop )
static ssize_t xread(int fd, void *buf, size_t count) {
	  // Read some bytes. Retry on EINTR and when we don't get as many bytes as we requested.
	  size_t alreadyRead = 0;
	  for(;;) {
		 ssize_t res = read(fd, buf, count);

		 if (res == -1 && errno == EINTR)
			 continue;

		 if (res > 0) {
		   buf = ((char*)buf)+res;
		   count -= res;
		   alreadyRead += res;
		 }

		 if (res == -1)
			 return -1;

		 if (count == 0 || res == 0)
			 return alreadyRead;
	}
}

/**
 * data dir reader
 * ex: `/proc`
 */
class Reader {
public:
	Reader(const std::string& dirname) {
		dir = opendir (dirname.c_str ());

		if (!dir) {
			throw std::bad_alloc();
		}

		struct dirent* entry;

		while ((entry = readdir(dir)) != NULL) {
			char* name = &entry->d_name[0];
			catalog.push_back (name);
		}
	}

	static inline std::shared_ptr<Reader> New(const std::string& dirname) {
		return std::make_shared<Reader>(dirname);
	}

	~Reader() {
		closedir (dir);
	}

	std::vector<std::string> list() {
		return catalog;
	}

private:
	DIR* dir;
	std::vector<std::string> catalog;
};

class Task : public pl::Process {
public:
	Task (const std::string &pid)
		: m_prefix("/proc/" + pid), m_pid(std::strtoul(pid.c_str (), 0, 10))
	{
		m_ppid		= 0;
		m_threads	= 0;
		m_state		= 0;
		m_pgrp		= 0;
		m_priority	= 0;
		m_size		= 0;

		readCmdline();
		readStat();
		readOvner();
		normalizeProcess();
	}

	~Task() {
	}

	inline std::string name () const override {
		return m_name;
	}

	inline uint32_t pid () const override {
		return m_pid;
	}

	inline uint32_t parentPid () const override {
		return m_ppid;
	}

	inline std::string path () const override {
		return m_path;
	}

	inline std::string cmdline() const {
		return m_cmdline;
	}

	inline std::string owner() const override {
		return m_owner;
	}

	inline uint32_t threads() const override {
		return m_threads;
	}

	inline int32_t priority () const override {
		return m_priority;
	}
	
private:
	std::string m_prefix;	// path to the process in procfs

	uint32_t	m_pid;
	uint32_t	m_ppid;			// pid of the parent process
	uint32_t	m_threads;	// number of threads

	std::string m_name;
	std::string m_path;
	std::string m_cmdline;

	char		m_state;			// state (R is running, S is sleeping, D is sleeping in an
												// uninterruptible wait, Z is zombie, T is traced or stopped)

	uint32_t	m_pgrp;			// pgrp of the process, to detect kernel thread
	int32_t		m_priority;	// priority level

	size_t		m_size;			// size of file, in bytes
	std::string	m_owner;


	void readCmdline() {
		int fd = open( (m_prefix + "/cmdline").c_str (), O_RDONLY);

		if (fd == -1) {
			return;
		}

		char command[4096+1];
		int amtRead = xread(fd, command, sizeof(command) - 1);

		close(fd);

		int tokenEnd = 0;
		if (amtRead > 0) {
			for (int i = 0; i < amtRead; i++)
				if (command[i] == '\0' || command[i] == '\n') {
					if (tokenEnd == 0) {
					   tokenEnd = i;
					}

					command[i] = ' ';
				}
		}

		if (tokenEnd == 0) {
			tokenEnd = amtRead;
		}

		command[amtRead] = '\0';

		m_cmdline = std::string(command, amtRead);
		m_path = std::string(command, tokenEnd);
	}

	void readStat() {
		int fd = open( (m_prefix + "/stat").c_str (), O_RDONLY);

		if (fd == -1) { return; }

		char buf[MAX_READ+1];
		int size = xread(fd, buf, MAX_READ);

		if (size <= 0) { return; }

		buf[size] = '\0';
		close(fd);

		std::string buffer(buf, size);

		char *location;

		if (m_pid != std::strtoul(buffer.c_str (), &location, 10)) {
			return;
		}

		location += 2;

		char *end = strrchr(location, ')');
		if (!end) { std::cerr << "error #2 "; return; }

		m_name = std::string(location, end);
		location = end + 2;

		m_state = location[0];
		location += 2;

		m_ppid  = std::strtoul(location, &location, 10);
		location += 1;

		m_pgrp = std::strtoul(location, &location, 10);
		location += 1;

		// skip parameters
		std::strtoul(location, &location, 10);
		location += 1;

		strtoul(location, &location, 10);
		location += 1;

		strtol(location, &location, 10);
		location += 1;

		strtoul(location, &location, 10);
		location += 1;

		strtoull(location, &location, 10);
		location += 1;

		strtoull(location, &location, 10);
		location += 1;

		strtoull(location, &location, 10);
		location += 1;

		strtoull(location, &location, 10);
		location += 1;

		strtoull(location, &location, 10);
		location += 1;

		strtoull(location, &location, 10);
		location += 1;

		strtoull(location, &location, 10);
		location += 1;

		strtoull(location, &location, 10);
		location += 1;

		m_priority = strtol(location, &location, 10);
		location += 1;

		// skip nice level
		strtol(location, &location, 10);
		location += 1;

		m_threads = strtol(location, &location, 10);
	}

	void readOvner() {
		struct stat sstat;

		int statok = stat(m_prefix.c_str (), &sstat);

		if (statok == -1) {
			return;
		}

		m_size = sstat.st_size;
		struct passwd* userData = getpwuid( sstat.st_uid );

		if (userData) {
			m_owner = std::string(userData->pw_name);
		}
	}

	void normalizeProcess() {
		char path[4096+1];

		ssize_t size = readlink((m_prefix + "/exe").c_str (), path, sizeof(path) - 1);

		if (size == -1) {
			return;
		}

		path[size] = '\0';
		m_path = std::string(path, size);
		m_name = std::string(basename(path));
	}
};

/**
 * get process list
 */
class Snapshot {
public:
	Snapshot() {
		std::shared_ptr<Reader> reader = Reader::New ("/proc");
		std::vector<std::string> data = reader->list ();

		// filter data, get only process pid
		std::copy_if(data.begin (), data.end (), std::back_inserter(m_pids), [](const std::string &pid){
			return is_pid(pid) && (pid != "0");
		});
	}

	inline static std::shared_ptr<Snapshot> New() {
		auto snap = std::make_shared<Snapshot>();
		snap->prepare();

		return std::move(snap);
	}

	/**
	 * construct <Task> from each found pid
	 */
	void prepare() {
		std::for_each(m_pids.begin (), m_pids.end(), [this](const std::string &pid){
			auto task = std::make_shared<Task>(pid);
			m_scope.push_back (std::move(task));
		});
	}

	/**
	 * process count
	 */
	inline size_t length() {
		return m_pids.size ();
	}

	inline pl::task::list_t tasks() const {
		return m_scope;
	}

private:
	std::vector<std::string> m_pids;
	pl::task::list_t m_scope;
};

namespace pl {
namespace task {

list_t list() {
	auto snapshot = Snapshot::New ();
	return snapshot->tasks ();
}

}

}
