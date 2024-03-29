#pragma once
#include <cstdint>
#include <ctime>
/*
Abstract classes implementing a file system
File systems are derived classes which implement the interface (and maybe more).
*/


/*
Represents a handle to a file. The file system returns a pointer to some
derived class of filehandler. The filehandler stores for instance the
position of a file on the disk, seek pos, permissions, and more generally all necessary information.

When destroyed, the filehandler gracefully closes the file.
*/
typedef size_t off_t;

enum seekref {SET, CUR, END}; // Tells us whether to offset from the beginning, current pos or end of a file.

struct statbuf { // Result from stat
	time_t tcreat;
	time_t tmod;

	int mode;
	uint16_t auth; // Authorization word; like Linux
};

class filehandler {
public:
	virtual size_t write(const void* buf, size_t count) = 0; // POSIX semantics
	virtual size_t read(void* buf, size_t count) = 0;
	
	virtual off_t seek(off_t offset, seekref whence) = 0;
	virtual statbuf stat() = 0;
};

class fs {
public:
	fs(filehandler*) = delete;
	// The filehandler given in argument represents the file on which lives the fs (e.g. /dev/sda)
	virtual filehandler* open(const char* path) = 0;
};
