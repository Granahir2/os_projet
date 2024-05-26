#pragma once
#include "kstdlib/memory.hh"
#include "kstdlib/string.hh"
#include "kernel/posix_except.hh"
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


#include "concepts.hh"

_STDEXCEPT(empty_directory, runtime_error)

class filehandler {

// perm_fh (defined in templates.hh) wraps an object which implements a file to give
// it mode semantics.
// filehandler can only be instantiated through perm_fh : mode semantics *must* be respected.
template<ImplementsFile T>
friend class perm_fh;
private:
	filehandler() = default;

public:
	virtual ~filehandler() = default;

	virtual size_t write(const void* buf, size_t count) = 0; // POSIX semantics
	virtual size_t read(void* buf, size_t count) = 0;
	
	virtual off_t seek(off_t offset, seekref whence) = 0;
	virtual statbuf stat() = 0;
};
/***********************************/

class dir_iterator {
public:
	virtual ~dir_iterator() = default;

	virtual drit_status operator<<(const char* str) = 0;
	virtual string get_canonical_path() = 0;

	virtual smallptr<filehandler> open_file([[maybe_unused]] const char* str,[[maybe_unused]] int mode) {return nullptr;}
	virtual string readlink([[maybe_unused]] const char* str) {return string();};

	virtual dirlist_token list() {return dirlist_token();}

	dir_iterator() = default;
};


class fs {
public:
	//fs(filehandler*);
	// The filehandler given in argument represents the file on which lives the fs (e.g. /dev/sda)
	
	virtual ~fs() = default;
	virtual dir_iterator* get_iterator() = 0;
};
