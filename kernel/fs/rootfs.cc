#include "rootfs.hh"
#include "kernel.hh"
#include "kstdlib/cstring.hh"
#include "kstdlib/cstdio.hh"

namespace rootfs {

drit_status rootfs_dit::operator<<(const char* str) {
	if(str == nullptr || str[0] == '\0') {return DIR_ENTRY;}

	if(stk_top == -1) {
		if(strcmp(str, ".") == 0 || strcmp(str, "..") == 0) {
			return DIR_ENTRY;
		}

		auto tolookup = '/' + string(str);
		int mntid = parent->lookup_mnt(tolookup.c_str());
		if (mntid != -1) {
			assert(++stk_top < 32, "[rootfs] Indirection overflow");
			drit_stk[stk_top] = {parent->mnts[mntid].fsys->get_iterator(), mntid};
			return DIR_ENTRY;
		} else {
			return NP;
		}
	} else {
		switch((*drit_stk[stk_top].di) << str) {
			case DIR_ENTRY:
				return DIR_ENTRY;
			case LINK_ENTRY: {
				string str = drit_stk[stk_top].di->readlink(str);
				auto p = str.c_str();
				assert(p, "[rootfs] readlink() failed");

				if(p[0] == '/') {
					auto it = parent->get_iterator();
					auto* curr = p+1;
					auto* next = strchr(p+1, '/');
					for(;next != nullptr; curr = next+1, next = strchr(next+1, '/')) {
						*next = '\0';
						if((*it) << curr != DIR_ENTRY) {return NP;}
						// Feed all the components one by one, checking link isn't broken
					}
					if((*it) << curr != DIR_ENTRY) {return NP;}
			
					// Swap the contents of it and this	
					auto t = it->stk_top;
					it->stk_top = stk_top;
					stk_top = t;	
					for(int i = 0; i < 32; ++i) {
						auto tmp = it->drit_stk[i];
						it->drit_stk[i] = drit_stk[i];
						drit_stk[i] = tmp;
					}
				} else {
					auto* curr = p+1;
					auto* next = strchr(p+1, '/');	
					for(;next != nullptr; curr = next+1, next = strchr(next+1, '/')) {
						*next = '\0';
						if(*this << curr != DIR_ENTRY) {return NP;}
						// Feed all the components one by one	
					}
					if(*this << curr != DIR_ENTRY) {return NP;};
				}
				}
				return DIR_ENTRY;
			case FILE_ENTRY:
				return FILE_ENTRY;
			case UNDERRUN:
				stk_top--;
				return DIR_ENTRY;
			case NP:
				auto tentative_mount = get_canonical_path() + string(str);
				int mntid = parent->lookup_mnt(tentative_mount.c_str());
				if (mntid != -1) {
					assert(++stk_top < 32, "[rootfs] Indirection overflow");
					drit_stk[stk_top] = {parent->mnts[mntid].fsys->get_iterator(), mntid};
					return DIR_ENTRY;
				} else {
					return NP;
				}
		}
	}
	assert(false, "Unreacheable !");
	return NP;
}

string rootfs_dit::get_canonical_path() {
	if(stk_top == -1) {
		auto r = string("/");
		return r;
	} else {
		return parent->mnts[drit_stk[stk_top].offset].mountpath
			+ drit_stk[stk_top].di->get_canonical_path();
	}
};

string rootfs_dit::readlink(const char* str) {
	if(stk_top == -1) {
		return string();
	} else {
		return drit_stk[stk_top].di->readlink(str);
	}
}

smallptr<filehandler> rootfs_dit::open_file(const char* str, int mode) {
	if(stk_top == -1) {
		return nullptr;
	} else {
		return drit_stk[stk_top].di->open_file(str, mode);
	}
}

dirlist_token rootfs_dit::list() {

	if(stk_top == -1) {
		dirlist_token* head = new dirlist_token {".", true, nullptr};
		head = new dirlist_token{"..", true, head, 0};
		for(int i = 0; i < 256; ++i) {
			if(!parent->mnts[i].fsys) continue;
			if(strchr(parent->mnts[i].mountpath.c_str()+1, '/') != nullptr) {
				continue;
			}
			head = new dirlist_token {parent->mnts[i].mountpath.c_str()+1, true, head, 0};
		}

		dirlist_token retval = {head->name, head->is_directory, std::move(head->next)};
		delete head;
		return retval;
	} else {
		return drit_stk[stk_top].di->list();
	}
}

int rootfs::lookup_mnt(const char* s) {
	for(int i = 0; i < 256;++i) {
		if(mnts[i].fsys && mnts[i].mountpath && strcmp(s, mnts[i].mountpath.c_str()) == 0) {
			return i;
		}
	}
	return -1;
}

void rootfs::mount(fs* tomount, const char* mountpath) {
	/* UGLY : we don't check existence of the path */
	int i = 0;
	for(; mnts[i].fsys && i < 256; ++i) {}
	if(i == 256) {throw eoverflow("Too many mounts!");}
	mnts[i].fsys = tomount;
	mnts[i].mountpath = mountpath;
}

rootfs::rootfs() {}

rootfs_dit* rootfs::get_iterator() {
	auto p = new rootfs_dit(this);
	return p;
}
}
