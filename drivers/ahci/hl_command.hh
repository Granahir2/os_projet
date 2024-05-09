#pragma once
#include "hba_defs.hh"
#include <cstddef>
#include "x86/memory.hh"
namespace ahci {

struct eff_comtab {
	uint8_t cfis[64];
	uint8_t acmd[16];
	uint8_t pad[0x80 - 0x50];
	phy_region_desc prdt[248];
}; static_assert(sizeof(eff_comtab) == 4096);

struct eff_command {
	eff_command(bool iswrite = false);
	~eff_command();

	void setup_linear_prdt(x64::phaddr start, size_t len);
	void setup_header_fis(volatile command_header* there, const void* fis, size_t fis_s);

	void reset(bool ndir);

	volatile command_header* head = nullptr;
	volatile eff_comtab* comtab;
	int curr_prdt_offset = -1;
	size_t totlen = 0;
	bool direction;
};


};
