#pragma once
#include "fs.hh"
#include "templates.hh"

struct _memf {
public:
	_memf(size_t sz) : buffer(new unsigned char[sz]), curr_offset(0), size(sz) {}
	size_t read(void* dst, size_t s) {
		if((size_t)curr_offset >= size) {return 0;}
		if(curr_offset + s >= size) {
			size_t delta = size  - curr_offset;
			memcpy(dst, &buffer[curr_offset], delta);
			curr_offset += delta;
			return delta;
		} else {
			memcpy(dst, &buffer[curr_offset], s);
			curr_offset += s;
			return s;
		}
	}
	size_t write(const void* src, size_t s) {
		if((size_t)curr_offset >= size) {return 0;}
		if((size_t)(curr_offset) + s >= size) {
			size_t delta = size - curr_offset;
			memcpy(&buffer[curr_offset], src, delta);
			curr_offset += delta;
			return delta;
		} else {
			memcpy(&buffer[curr_offset], src, s);
			curr_offset += s;
			return s;
		}
	}
	off_t seek(off_t off, seekref ref) {
		switch(ref) {
			case SET:
				if(off >= 0 && (size_t)off <= size) {
					return (curr_offset = off);
				} else {
					throw einval("Out of bounds seek");
				}
			case CUR:
				return seek(off + curr_offset, SET);
			case END:
				return seek(off + size, SET); 
			default:
				throw einval("Unknown reference pos for seek");
		}
	} 

private:
	smallptr<unsigned char[]> buffer;
	off_t curr_offset;
	size_t size;
};

using memfile = perm_fh<_memf>;
