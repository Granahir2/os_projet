#pragma once

struct __attribute__((packed)) memory_map_entry {
	uint64_t base;
	uint64_t length;
	enum {NONE, AVAILABLE, RESERVED, ACPI_RECLAIMABLE, NVS, BADRAM} type;
};
