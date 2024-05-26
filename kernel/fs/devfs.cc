#include "devfs.hh"
#include "kstdlib/cstdlib.hh"
#include "kstdlib/cstdio.hh"
#include "memf.hh"

drit_status _devfs_dit::push(const char* str) {	
	if(str == strstr(str, "ser")) {
		long l = strtol(str, nullptr, 10);
		if(l >= 0 && l < 32 && parent->pdarr[l]) {
			return FILE_ENTRY;
		}
		return NP;
	} else if(strcmp(str, "tty0") == 0 && parent->td){
		return FILE_ENTRY;
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
	} else if(strcmp(str, "tty0") == 0 && parent->td) { // Only one tty for now
		return (parent->td.ptr)->get_file(mode);
	} else {
		return nullptr;
	}
}

devfs_dit* devfs::get_iterator() {
	return new devfs_dit(this);
}

dirlist_token _devfs_dit::list() {
	dirlist_token *curr = nullptr;
	for(int i = 0; i < 32; ++i) {
		if(parent->pdarr[i]) {
			memfile f(RW, 30);
			fprintf(&f, "ser%d\0", i);
			char buf[10];
			buf[9] = '\0';
			f.seek(0, SET);
			f.read(&buf[0], 10);
			curr = new dirlist_token{buf, false, curr};
		}
	}

	if(parent->td) {
		curr = new dirlist_token{"tty0", false, curr};
	}
	if(curr == nullptr) {throw empty_directory();}
	dirlist_token base = {curr->name, curr->is_directory, std::move(curr->next), curr->file_size};
	return base;
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

bool devfs::attach_tty(smallptr<vga::text_driver>&& x) {
	if(!td) {
		td.swap(std::move(x));
		return true;
	}
	return false;
}
