#include "ahci.hh"
#include "kstdlib/cstdio.hh"
#include "kernel/kernel.hh"
#include "kernel/mem/utils.hh"

#include "hl_command.hh"
namespace ahci {
driver::driver(filehandler* cf) {
	uint32_t config[64];
	cf->read(config, 256);

	unsigned dtype = (config[0x2] >> 16) & 0xffff;

	if(dtype != 0x0106) {
		printf("Device type %x\n", dtype);
		throw runtime_error("[ahci] Invalid device type.");
	}

	uint32_t abar = config[0x9];
	if(abar & 0xfff) {
		throw runtime_error("[ahci] ABAR structure doesn't follow spec");
	}
	hmem = (HBA_memory*)((uintptr_t)(abar) - 512*1024*1024*1024ul);
	printf("[ahci] Found ABAR at : %p\n", hmem);


	int cap = (config[0xd] & 0xff) & ~0b11; // We are on a modern device
	while((config[cap>>2]&0x0f) != 0x05) {
		cap = (config[cap>>2] >> 8) & 0xff;
		if(!cap) { // End of the list
			throw runtime_error("[ahci] Couldn't find MSI capability");
		}
	}
	printf("[ahci] HBA wants : %d MSI slots\n", (config[msicap] >> 16) & 0xe);
	
	/*
		TODO : implement MSI interrupt routines ?
	*/

	msicap = cap;
	conf = cf;	

	hmem->handoff_bios_os |= 0x2;
	while(hmem->handoff_bios_os & 0x1) {}
	hmem->global_host_ctrl |= 0x80000001; // AE and RST
	hmem->global_host_ctrl &= ~0x2; // disable interrupts
	uint32_t newcommand = config[0] | 0b111;
	cf->seek(0, SET);
	cf->write(&newcommand, sizeof(newcommand));
}


smallptr<port_drv> driver::get_port(int portnum) {
	if(portnum >= 32) throw einval();

	if(hmem->port_impl_btmp & (1u << portnum)) {
		return smallptr<port_drv>(new port_drv(portnum, this));
	}

	throw enxio("[ahci] Port not implemented");
}

port_drv::port_drv(int portnum, driver* parent) : pnum(portnum), p(parent), port(&(parent->hmem->port[portnum])) {
	/* portnum implemented is checked by the parent */
	if((port->SATA_status & 0xf) != 0x3) {
		throw enxio("[ahci] Invalid device in port");
	}
	printf("Port %d status %x\n", portnum, port->SATA_status);
	if(port->cmd & (1u << 24)) {
		throw einval("[ahci] ATAPI not supported");
	}
	port->cmdlist_base = (command_header*)get_phpage();


	port->interrupt_status = (uint32_t)(-1);
	port->cmd |= 0x10; // Start command processing if not already happening
	port->cmd |= 0x01;
	while(port->interrupt_status == 0) {} // Wait for first D2H comm

	printf("[ahci] Device signature : %x\n", port->signature);
	port->interrupt_status = (uint32_t)(-1);

	fis::reg_h2d identify;
	identify.command = 0xec;
	identify.config |= 0x80; // Command

	eff_command id(false);
	uint16_t recv_buf[0x100];
	id.setup_linear_prdt(mem::resolve_to_phmem((x64::linaddr)(recv_buf)).resolved, 512);
	id.setup_header_fis(&port->cmdlist_base[0], &identify, sizeof(identify));
	port->command_issue = 1; // Start processing

	puts("[ahci] Sent ATA IDENTIFY\n");
	while((port->command_issue & 1) && port->interrupt_status == 0) {}

	total_size = *(uint64_t*)(&recv_buf[100]);
	printf("[ahci] Drive supports %lu LBAs\n", total_size);
}

/* TODO add error handling; check for errors in each function call*/
int port_drv::start_read(volatile void* buffer, size_t at, uint16_t numsec) {
	port->interrupt_status = 0xffffffff;

	fis::reg_h2d read;
	read.command = 0x25;
	read.config |= 0x80;
	read.lbal24_device   = (at & 0xffffff) | 0x40000000;
	read.lbah24_featureh = (at >> 24) & 0xffffff; 
	read.count = numsec;

	uint32_t curr_running = port->command_issue;
	unsigned slot = 0;
	for(; slot <= ((p->hmem->capability >> 8) & 0x1f) && (curr_running >> (slot) & 1); ++slot) {}
	if(slot > (p->hmem->capability >> 8 & 0x1f)) {throw ebusy();}

	cmdslot[slot].reset(false); // Add provisions for more than page-sized buffers (may not be phy contiguous) ?
	cmdslot[slot].setup_linear_prdt(mem::resolve_to_phmem((x64::linaddr)(buffer)).resolved, numsec * 512);
	cmdslot[slot].setup_header_fis(&port->cmdlist_base[slot], &read, sizeof(read));

	port->command_issue = (1u << slot);
	return slot;	
}


int port_drv::start_write(volatile const void* buffer, size_t at, uint16_t numsec) {
	port->interrupt_status = 0xffffffff;

	fis::reg_h2d write;
	write.command = 0x35;
	write.config |= 0x80;
	write.lbal24_device   = (at & 0xffffff) | 0x40000000;
	write.lbah24_featureh = (at >> 24) & 0xffffff; 
	write.count = numsec;

	uint32_t curr_running = port->command_issue;
	unsigned slot = 0;
	for(; slot <= ((p->hmem->capability >> 8) & 0x1f) && (curr_running >> (slot) & 1); ++slot) {}
	if(slot > (p->hmem->capability >> 8 & 0x1f)) {throw ebusy();}

	cmdslot[slot].reset(true);
	cmdslot[slot].setup_linear_prdt(mem::resolve_to_phmem((x64::linaddr)(buffer)).resolved, numsec * 512);
	cmdslot[slot].setup_header_fis(&port->cmdlist_base[slot], &write, sizeof(write));

	port->command_issue = (1u << slot);
	return slot;	
}

bool port_drv::check_on_read(int rddesc) {
	// Check for errors...
	if(port->interrupt_status & 0xff000000) { // If there was some kind of error
		printf("Got IF : %x\n", port->interrupt_status);
		printf("SErr : %x\n", port->SATA_err);
		printf("CI : %x\n", port->command_issue);
	}
	port->interrupt_status = 0xffffffff;
	return !(port->command_issue & (1 << rddesc));
}

};
