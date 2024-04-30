#pragma once
#include "concepts.hh"


/*TODO : integrate files from which you can't read/write instead of having the driver
handle this.
*/
template<ImplementsFile T>
class perm_fh : public filehandler {
public:
	template<class... U>
	perm_fh(mode_t mode, U... args) : m(mode), raw_f(args...) {};

	size_t write(const void* buf, size_t count) override {
		if(m & WRONLY) { return raw_f.write(buf, count);}
		else throw eperm("Unauthorized write");
	}
	size_t read(void* buf, size_t count) override {
		if(m & RDONLY) { return raw_f.read(buf, count);}
		else throw eperm("Unauthorized read");
	}

	off_t seek(off_t offset, seekref whence) override {
		if constexpr (Seekable<T>) {
			return do_seek(offset, whence);
		} else {
			throw einval("Seek on unseekable file");
		}
	}

	statbuf stat() {return statbuf();}

	const mode_t m;
	T raw_f;

private:
off_t do_seek(off_t offset, seekref whence) requires requires { {raw_f.seek(offset, whence)} -> std::same_as<off_t>; } {
	return raw_f.seek(offset, whence);
}
};


/*
Compile-time wrapper for dir_iterator which ensures the semantics of . and .. are respected.
Also a convenience wrapper.

The facilities of the raw iterator are preserved through raw_it.
Using raw_it directly is discouraged however, and should only
be done for accessing special features of the iterator, *not* doing path resolution.

It is recommended that most filesystems implement their directory iterator through coh_dit,
but this is not mandatory to accomodate exotic semantics.
*/
template<ImplementsDirIterator T>
class coh_dit : public dir_iterator {
public:
	template<typename... U>
	coh_dit(U... args) : raw_it(args...) {};

	drit_status operator<<(const char* str) override {
		if(str[0] == '.') {
			if(str[1] == '.' && str[2] == '\0') {
				if(raw_it.depth()) {
					raw_it.pop();
					return DIR_ENTRY;
				} else {
					return UNDERRUN;	
				}
			} else if(str[1] == '\0') {
				return DIR_ENTRY;
			}
		}
		return raw_it.push(str);
	}

	string get_canonical_path() override { // REALLY SLOW
		string acc = string("/");
		for(size_t i = 0; i < raw_it.depth(); i++) {
			acc = acc + raw_it[i] + string("/"); // raw_it[i] must be convertible to string
		}
		return acc;
	}

	smallptr<filehandler> open_file(const char* str, int mode) override {
		if(raw_it.push(str) == FILE_ENTRY) {
			return raw_it.open_file(str, mode);
		} else {
			throw enoent("File doesn't exist");
		}
	}

	T raw_it;
};
