#pragma once
#include "fs.hh"

/*
Root file system. Handles mount points and provides a uniform interface.
Integrates link following.
*/

struct rootfs_dit : dir_iterator {
public:
	rootfs_dit() : stk_top(-1) {};
	void operator<<(const char* str) override;
	const char* get_canonical_path() = 0;
private:
	
	enum {dir_iterator* di, int offset} drit_stk[32] = {{nullptr, 0}};
	
	int stk_top = -1;
	rootfs* parent;
}
