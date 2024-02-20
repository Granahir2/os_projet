#pragma once

#include "x86/descriptors.hh"
#include "x86/memory.hh"
#include <cstdint>

struct interrupt_manager {
public:
	interrupt_manager();
	void enable();

	void register_IRQ(uint8_t irq, uint8_t number, x64::linaddr isr);
	void register_gate(uint8_t vector, uint8_t ist, x64::linaddr isr);
	
	uint64_t apic_base();
	uint32_t apic_id();
	uint32_t apic_version();
	uint32_t ioapic_version();
private:
	static x64::gate_descriptor idt[256]; // We don't worry about thread safety YET.
	
	static constexpr x64::idt_descriptor idt_desc = {
		sizeof(x64::gate_descriptor) * 256,
		(x64::linaddr)(&idt[0])};

	static bool isinit;

	uint32_t volatile* apic;
	struct IRQ_alloc {
		uint8_t irq;
		uint8_t vector;
		x64::linaddr israrr;
	};

	IRQ_alloc irqallocs[256]; // IRQ allocations can be moved through the IOAPIC
};
