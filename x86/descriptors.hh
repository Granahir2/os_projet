#pragma once
#include <stdint.h>
#include "memory.hh"

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
		uint8_t flags : 4 = 0; // low bits first
		uint8_t base_hi = 0;

		constexpr void set_limit(uint32_t limit) {
			limit_lo = limit & 0xffff;
			limit_hi = (limit >> 16) & 0xf;
		}
		constexpr uint32_t get_limit() const {
			return limit_lo | ((uint32_t)(limit_hi) << 16);
		}

		constexpr void set_base(uint32_t base)  {
			base_lo = base & 0xffff;
			base_mid = (base >> 16) & 0xff;
			base_hi = (base >> 24) & 0xff;
		}
		constexpr uint32_t get_base() const
			{return base_lo | ((uint32_t)(base_mid) << 16) | ((uint32_t)(base_hi) << 24);}
	}; static_assert(sizeof(segment_descriptor) == 8);
}

namespace x64 {
	struct __attribute__((packed)) gdt_descriptor {
		uint16_t size;
		linaddr base;
	}; static_assert(sizeof(gdt_descriptor) == 10);	
	
	struct __attribute__((packed)) idt_descriptor {
		uint16_t size;
		linaddr base;
	}; static_assert(sizeof(idt_descriptor) == 10);

	using segment_descriptor = x86::segment_descriptor;
	
	struct __attribute__((packed)) system_segment_descriptor {
		struct x86::segment_descriptor inner;
		uint32_t extended_base;
		uint32_t reserved;
		
		constexpr void set_base(linaddr base)  {
			inner.base_lo = base & 0xffff;
			inner.base_mid = (base >> 16) & 0xff;
			inner.base_hi = (base >> 24) & 0xff;
			extended_base = base >> 32;
		}

		constexpr void set_limit(uint32_t limit) {
			inner.limit_lo = limit & 0xffff;
			inner.limit_hi = (limit >> 16) & 0xf;
		}
	};

	struct __attribute__((packed)) TSS {
		uint32_t reserved_0 = 0;
		linaddr rsp[3] = {0}; // Stack pointer loaded upon priviledge upgrade
		linaddr ist[8] = {0}; // Stack table; used for interrupts
		uint64_t reserved_1 = 0;
		uint16_t reserved_2 = 0;
		uint16_t iopb = sizeof(TSS);
	}; static_assert(sizeof(TSS) == 104);

	struct __attribute__((packed)) gate_descriptor {
		uint16_t offset_lo = 0;
		uint16_t cs_segment_selector = 0;
		uint8_t  ist_value = 0;
		uint8_t  attributes = 0x0e; // P DPL 0 1 1 1 trap
		uint16_t offset_mid = 0;
		uint32_t offset_hi = 0;
		uint32_t reserved = 0;
	}; static_assert(sizeof(gate_descriptor) == 16);
};
