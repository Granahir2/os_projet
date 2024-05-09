#pragma once

namespace ahci {
namespace fis {

// We do not define unused FIS types
// (DMA activate, PIO, DATA, BIST)
struct __attribute__((packed)) dma_setup {
	uint8_t fis_type = 0x41; // Must be 0x41
	
	uint8_t config = 0;
	uint16_t rsvd0 = 0;

	uint64_t buffer_id = 0; // Unused by AHCI ?

	uint32_t rsvd1 = 0;
	uint32_t buffer_offset = 0; // dword aligned
	uint32_t xfer_size = 0;

	uint32_t rsvd2 = 0;
};

struct __attribute__((packed)) reg_h2d {
	uint8_t fis_type = 0x27; // Must be 0x27
	uint8_t config = 0;

	uint8_t command = 0;
	uint8_t featurel = 0;

	uint32_t lbal24_device = 0;
	uint32_t lbah24_featureh = 0;

	uint16_t count = 0;

	uint8_t icc = 0;
	uint8_t control = 0;

	uint32_t rsvd0 = 0;
};

struct __attribute__((packed)) reg_d2h {
	uint8_t fis_type = 0x34; // Must be 0x34
	uint8_t config;

	uint8_t status;
	uint8_t error;

	uint32_t lba24_device;
	uint32_t lba_high;

	uint16_t count;

	uint16_t rsvd0;
	uint32_t rsvd1;
};

struct __attribute__((packed)) pio_setup {
	uint8_t fis_type = 0x5f; // Must be 0x5f
	uint8_t config;	
	uint8_t status;
	uint8_t error;

	uint32_t lba_low_device;
	uint32_t lba_high;

	uint16_t count;

	uint8_t rsvd0;
	uint8_t e_status;

	uint16_t tc;
	uint16_t rsvd1;
};

}
}
