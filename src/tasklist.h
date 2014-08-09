#ifndef TASKLIST_H
#define TASKLIST_H

#include <vector>
#include <memory>
#include <string>
#include <stdint.h>

class Process {
public:
	~Process ();

	virtual std::string name () const = 0;	  // short name of process, executable file name
	virtual uint32_t pid () const = 0;        // process PID
	virtual uint32_t parentPid () const = 0;  // parent process PID
	virtual std::string path () const = 0;	  // full path to process image
};

std::vector< std::shared_ptr<Process> > tasklist ();

#endif