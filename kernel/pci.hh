#pragma once

#include <stdint.h>

namespace pci {
	struct header {
		uint16_t vendor_id;
		uint16_t device_id;
		uint16_t command;
		uint16_t status;
		uint8_t  rev_id;
		uint8_t  prog_if;
		uint8_t  subclass;
		uint8_t  devclass;
		uint16_t deprecated; // Really ?
		uint8_t  header_type;
		uint8_t  BIST;		
	};

	struct pci_addr {
		uint8_t bus;
		uint8_t device;
	};

	void enumerate();
}
