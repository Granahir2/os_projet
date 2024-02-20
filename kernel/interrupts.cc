#include "interrupts.hh"
#include "x86/asm_stubs.hh"


bool interrupt_manager::isinit;
x64::gate_descriptor interrupt_manager::idt[256]; // We don't worry about thread safety YET.

void ioapic_write(uint32_t reg, uint32_t data) {
	uint32_t volatile* const ioapic_addr = (uint32_t*)0xfec00000;
	uint32_t volatile* const ioapic_data = (uint32_t*)0xfec00010;

	*ioapic_addr = reg;
	*ioapic_data = data; 
}

uint32_t ioapic_read(uint32_t reg) {
	uint32_t volatile* const ioapic_addr = (uint32_t*)0xfec00000;
	uint32_t volatile* const ioapic_data= (uint32_t*)0xfec00010;

	*ioapic_addr = reg;
	return *ioapic_data;
}

interrupt_manager::interrupt_manager() { 
	apic = (uint32_t volatile*)(x64::rdmsr(0x1b) & 0xffffff000);
	if(!isinit) {
		isinit = true;

		/* Setup a do-nothing IDT */
		for(int i = 0; i < 256; ++i) {
			idt[i] = x64::gate_descriptor();
		}

		x64::outb(0x21, 0xff); // Disable the PICs
		x64::outb(0xa1, 0xff);

		/* Setup a do-nothing local APIC */
		
		constexpr uint32_t disabled_lvt_entry = 0x10000;
		for(int off = 0x320/4; off <= 0x370/4; off+=0x10/4) {
			apic[off] = disabled_lvt_entry;
		}
		apic[0x2f0/4] = disabled_lvt_entry;
		
		/* Setup a do-nothing IO APIC */
		constexpr uint32_t disabled_redir_entry_lo = 0x10000;
		uint32_t num_relocs = ioapic_read(0x01);
		for(uint32_t off = 0; off < num_relocs; ++off) {
			ioapic_write(off*2 + 0x13, disabled_redir_entry_lo);
		}
		
	}
}

void interrupt_manager::register_gate(uint8_t vector, uint8_t ist, x64::linaddr isr) {
	idt[vector].attributes |= 0x80;
	idt[vector].offset_lo = isr & 0xffff;
	idt[vector].cs_segment_selector = 0x08;
	idt[vector].offset_mid = (isr >> 16) & 0xff;
	idt[vector].ist_value = ist;
	idt[vector].offset_hi = isr >> 32;
}

uint64_t interrupt_manager::apic_base() {
	return (uint64_t)(apic);
}
uint32_t interrupt_manager::apic_id() {
	return apic[0x20/4];
}
uint32_t interrupt_manager::apic_version() {
	return apic[0x30/4];
}
uint32_t interrupt_manager::ioapic_version() {
	return ioapic_read(0x01);
}
void interrupt_manager::enable() {
	x64::lidt((x64::linaddr)&idt_desc);
	x64::sei();
	apic[0x2f0/4] |= 0x1ff;
}
