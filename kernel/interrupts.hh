#pragma once

#include "x86/descriptors.hh"
#include "x86/memory.hh"

struct interrupt_manager {
public:
	interrupt_manager();
	void enable();
	void disable();

	/* Naively allocate MSIs. There is no support for de-allocation.
	There is currently no support for distinguishing traps/interrupts.
	*/
	uint8_t register_interrupt_block(uint8_t number, x64::linaddr* isrlist);
	//uint8_t register_IO_interrupt(uint8_t irqnum, x64::linaddr isr); // Used to register interrupts through I/O APIC
	void register_gate(uint8_t vector, uint8_t ist, x64::linaddr isr);
	
	uint64_t apic_base();
	uint32_t apic_id();
	uint32_t apic_version();
	uint32_t ioapic_version();

	void send_EOI();
private:
	static x64::gate_descriptor idt[256]; // We don't worry about thread safety YET.
	
	static constexpr x64::idt_descriptor idt_desc = {
		sizeof(x64::gate_descriptor) * 256,
		(x64::linaddr)(&idt[0])};

	static bool isinit;
	static uint32_t isallocd[7]; // Bitmask of allocated vectors in the range 0x20-0xff

	uint32_t volatile* apic;
};
