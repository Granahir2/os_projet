#pragma once
#include "hba_defs.hh"
#include "hl_command.hh"
#include "kernel/fs/fs.hh"
namespace ahci {
class driver;

class port_drv {
public:
	port_drv(int portnum, driver* parent);
	
	int start_read(volatile void* buffer,size_t at, uint16_t numsector);
	bool check_on_read(int rddesc);

	int start_write(volatile const void* buffer, size_t at, uint16_t numsector);

	uint64_t total_size = 0;
private:
	eff_command cmdslot[32];
	uint32_t running_cmd = 0;


	int pnum;
	driver* p;
	volatile HBA_port* port;
	command_header* cmdlist;
};

class driver {
friend port_drv;
public:
	driver(filehandler* conffile);
	smallptr<port_drv> get_port(int portnum);
private:
	volatile HBA_memory* hmem;
	filehandler* conf;
	int msicap;	
};


}
