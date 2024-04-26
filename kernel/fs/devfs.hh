#pragma once
#include "fs.hh"
#include "drivers/serial/serial.hh"
#include "drivers/vga_text/vga_text.hh"

class devfs;

class _devfs_dit {
friend devfs;
public:
	_devfs_dit(devfs* p) : parent(p) {}
	void pop() {};
	drit_status push(const char* str);
	size_t depth() {return 0;}
	string operator[](size_t) {return string();}
	smallptr<filehandler> open_file(const char* s, int mode);
private:
	devfs* parent = nullptr;
};

using devfs_dit = coh_dit<_devfs_dit>;

class devfs : public fs {
friend _devfs_dit;
public:
	devfs_dit* get_iterator() override;
	
	bool attach_serial(smallptr<serial::portdriver>&&); // only device type supported as of now
	bool attach_tty(smallptr<vga::text_driver>&&);
private:
	smallptr<serial::portdriver>  pdarr[32] = {smallptr<serial::portdriver>()}; // probably dynamic in future iteration
	smallptr<vga::text_driver> td = smallptr<vga::text_driver>();
};
