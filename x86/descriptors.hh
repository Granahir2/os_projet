#pragma once

namespace x86 {
	struct __attribute__((packed)) gdt_descriptor {
		uint16_t size = 0;
		uint32_t offset = 0;	
	}; static_assert(sizeof(gdt_descriptor) == 6);

	struct __attribute__((packed)) segment_descriptor  {
		uint16_t limit_lo = 0;
		uint16_t base_lo = 0;
		uint8_t base_mid = 0;
		uint8_t access = 0;
		
		uint8_t limit_hi : 4 = 0;
		uint8_t flags : 4 = 0; // high bits first
		uint8_t base_hi = 0;

		constexpr void set_limit(uint32_t limit) {
			limit_lo = limit & 0xffff;
			limit_hi = (limit >> 16) & 0xf;
		}
		constexpr uint32_t get_limit() {
			return limit_lo | ((uint32_t)(limit_hi) << 16);
		}

		constexpr void set_base(uint32_t base) {
			base_lo = base & 0xffff;
			base_mid = (base >> 16) & 0xff;
			base_hi = (base >> 24) & 0xff;
		}
		constexpr uint32_t get_base()
			{return base_lo | ((uint32_t)(base_mid) << 16) | ((uint32_t)(base_hi) << 24);}
	}; static_assert(sizeof(segment_descriptor) == 8);
}

namespace x64 {
	struct __attribute__((packed)) gdt_descriptor {
		x86::gdt_descriptor inner;
		uint32_t upper_offset = 0;
	}; static_assert(sizeof(gdt_descriptor) == 10);
	
	struct system_segment_descriptor {
		struct x86::segment_descriptor inner;
		uint32_t extended_base;
		uint32_t reserved;	
	};
};
