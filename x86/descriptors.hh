#pragma once

namespace x86 {
	struct gdt_descriptor {
		uint16_t size;
		uint32_t offset;	
	};

	struct segment_descriptor {
		uint16_t limit_lo;
		uint16_t base_lo;
		uint8_t base_mid;
		uint8_t access;
		uint8_t flags : 4; // high bits first
		uint8_t limit_hi : 4;
		uint8_t base_hi;

		void set_limit(uint32_t limit) {
			limit_lo = limit & 0xffff;
			limit_hi = (limit >> 16) & 0xf;
		}
		uint32_t get_limit() {
			return limit_lo | ((uint32_t)(limit_hi) << 16);
		}

		void set_base(uint32_t base) {
			base_lo = base & 0xffff;
			base_mid = (base >> 16) & 0xff;
			base_hi = (base >> 24) & 0xff;
		}
		uint32_t get_base() {return base_lo | ((uint32_t)(base_mid) << 16) | ((uint32_t)(base_hi) << 24);}
	};
}

namespace x64 {
	struct gdt_descriptor {
		uint16_t size;
		uint64_t offset;
	};
	
	struct system_segment_descriptor {
		struct x86::segment_descriptor inner;
		uint32_t extended_base;
		uint32_t reserved;	
	};
};
