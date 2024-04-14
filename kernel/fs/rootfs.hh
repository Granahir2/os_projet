#pragma once
#include "fs.hh"
#include "kstdlib/string.hh"
/*
Root file system. Handles mount points and provides a uniform interface.
Integrates link following.
*/

namespace rootfs {
struct rootfs;

struct rootfs_dit : public dir_iterator {
friend rootfs;
public:
	rootfs_dit() : stk_top(-1) {};
	rootfs_dit(rootfs* p) : stk_top(-1), parent(p) {};
	drit_status operator<<(const char* str) override;
	string get_canonical_path() override;
	string readlink(const char* str) override;
	smallptr<filehandler> open_file(const char* str, int mode) override; 
private:
	
	struct {dir_iterator* di; int offset;} drit_stk[32] = {{nullptr, 0}};
	
	int stk_top = -1;
	rootfs* parent;
};

struct rootfs : public fs {
friend rootfs_dit;
public:
	rootfs();
	rootfs_dit* get_iterator() override;

	int lookup_mnt(const char* str);
	struct mntentry {
		mntentry() : fsys(nullptr), mountpath() {}
		fs* fsys = nullptr;
		string mountpath;
	};
	mntentry mnts[256];
};
}
