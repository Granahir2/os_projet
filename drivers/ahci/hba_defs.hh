#pragma once
#include <cstdint>

namespace ahci {

struct command_header;
struct recv_fis;

struct __attribute__((packed)) HBA_port {
	volatile command_header* cmdlist_base;
	volatile recv_fis* recv_fis_base;

	uint32_t interrupt_status;
	uint32_t interrupt_enable;

	uint32_t cmd;
	uint32_t rsv0;

	uint32_t taskfile_dat;
	uint32_t signature;

	uint32_t SATA_status;
	uint32_t SATA_ctl;
	uint32_t SATA_err;

	uint32_t SATA_active;
	uint32_t command_issue;

	uint32_t SATA_notif;

	uint32_t FIS_switch_ctl;
	uint32_t rsv1[15];
}; static_assert(sizeof(HBA_port) == 0x80);

struct __attribute__((packed)) HBA_memory {
	uint32_t capability;
	uint32_t global_host_ctrl;
	uint32_t interrupt_status;
	uint32_t port_impl_btmp;
	uint32_t version;

	uint32_t cc_coalesce_ctrl;
	uint32_t cc_coalesce_port;

	uint32_t em_loc;
	uint32_t em_ctl;

	uint32_t capability_ext;
	uint32_t handoff_bios_os;

	uint8_t rsv[0x100 - 0x2c];
	volatile HBA_port port[32];
};
}

#include "fis.hh"
#include "x86/memory.hh"
namespace ahci {

struct __attribute__((packed)) recv_fis {
	fis::dma_setup ds;
	uint32_t pad0;

	fis::pio_setup ps;
	uint32_t pad1[3];

	fis::reg_d2h reg_recv;
	uint32_t pad2;

	uint8_t fpad[0x100 - 0x58];
};
static_assert(sizeof(recv_fis) == 0x100);

struct command_header {
	uint8_t  cfl:5;		// Command FIS length in DWORDS, 2 ~ 16
	uint8_t  a:1 = 0;		// ATAPI
	uint8_t  w:1 = 0;		// Write, 1: H2D, 0: D2H
	uint8_t  p:1 = 0;		// Prefetchable
 
	uint8_t  r:1 = 0;		// Reset
	uint8_t  b:1 = 0;		// BIST
	uint8_t  c:1 = 0;		// Clear busy upon R_OK
	uint8_t  rsv0:1 = 0;		// Reserved
	uint8_t  pmp:4 = 0;		// Port multiplier port

	uint16_t prdt_entrylen;
	volatile uint32_t xferred_bytecnt;

	x64::phaddr command_table;

	uint32_t rsv1[4];
}; static_assert(sizeof(command_header) == 8*sizeof(uint32_t));

struct phy_region_desc {
	x64::phaddr loc = 0;
	uint32_t pad = 0;

	uint32_t dbc:22 = 0; // 1-based
	uint32_t rsv1:9 = 0;
	uint32_t ie:1 = 0;
};
};
