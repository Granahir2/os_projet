#include "devfs.hh"
#include "kstdlib/cstdlib.hh"
#include "kstdlib/cstdio.hh"

drit_status _devfs_dit::push(const char* str) {	
	if(str == strstr(str, "ser")) {
		long l = strtol(str, nullptr, 10);
		if(l >= 0 && l < 32 && parent->pdarr[l]) {
			return FILE_ENTRY;
		}
		return NP;
	} else {
		return NP;
	}
}

smallptr<filehandler> _devfs_dit::open_file(const char* str, int mode) {
	if(str == strstr(str, "ser")) {
		long l = strtol(str, nullptr, 10);
		if(l >= 0 && l < 32 && parent->pdarr[l]) {
			return (parent->pdarr[l].ptr)->get_file(mode);
		}
		return nullptr;
	} else {
		return nullptr;
	}
}

devfs_dit* devfs::get_iterator() {
	return new devfs_dit(this);
}

bool devfs::attach_serial(smallptr<serial::portdriver>&& x) {
	for(int i = 0; i < 32; i++) {
		if(!pdarr[i]) {
			pdarr[i].swap(std::move(x));
			return true;
		}
	}
	return false;
}
