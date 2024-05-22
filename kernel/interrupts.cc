#include "interrupts.hh"
#include "x86/asm_stubs.hh"


bool interrupt_manager::isinit;
uint32_t interrupt_manager::isallocd[7];
x64::gate_descriptor interrupt_manager::idt[256]; // We don't worry about thread safety YET.

void ioapic_write(uint32_t reg, uint32_t data) {
	uint32_t volatile* const ioapic_addr = (uint32_t*)(0xfec00000 - 512*1024*1024*1024ul);
	uint32_t volatile* const ioapic_data = (uint32_t*)(0xfec00010 - 512*1024*1024*1024ul);

	*ioapic_addr = reg;
	*ioapic_data = data; 
}

uint32_t ioapic_read(uint32_t reg) {
	uint32_t volatile* const ioapic_addr = (uint32_t*)(0xfec00000 - 512*1024*1024*1024ul);
	uint32_t volatile* const ioapic_data= (uint32_t*)(0xfec00010 - 512*1024*1024*1024ul);

	*ioapic_addr = reg;
	return *ioapic_data;
}

interrupt_manager::interrupt_manager() { 
	apic = (uint32_t volatile*)((x64::rdmsr(0x1b) & 0xffffff000) - 512*1024*1024*1024ul);
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

void interrupt_manager::register_gate(uint8_t vector, uint8_t ist, x64::linaddr isr, uint8_t pl) {
	idt[vector].attributes |= 0x80 | (pl << 5);
	idt[vector].offset_lo = isr & 0xffff;
	idt[vector].cs_segment_selector = 0x08;
	idt[vector].offset_mid = (isr >> 16) & 0xffff;
	idt[vector].ist_value = ist;
	idt[vector].offset_hi = isr >> 32;

	if(vector >= 32) {
		isallocd[(vector - 32)/32] |= 1u << ((vector - 32) % 32);
	}

}

uint32_t volatile* interrupt_manager::apic_base() const { return apic; }
uint32_t interrupt_manager::apic_id() {return apic[0x20/4];}
uint32_t interrupt_manager::apic_version() {return apic[0x30/4];}
uint32_t interrupt_manager::ioapic_version() { return ioapic_read(0x01);}
void interrupt_manager::enable() {
	// Setup the spurious interrupt vector ? 
	x64::lidt((x64::linaddr)&idt_desc);
	x64::sei();
	apic[0x2f0/4] |= 0x1ff;
}
void interrupt_manager::disable() {x64::cli();}
void interrupt_manager::send_EOI() {apic[0xb0/4] = 0;}

uint8_t interrupt_manager::register_interrupt_block(uint8_t number, x64::linaddr* irq) {
	
	bool isokay = false;
	for(int i = 1; i <= 32; i*=2) {
		if(i == number) {isokay = true; break;}
	}
	if(!isokay) {return 0x00;}

	for(int i = 0; i < 7; ++i) {
	for(int j = 0; j < 32; j += number) {
		if(((isallocd[i] >> j) & (2*number - 1)) == 0) {
			uint8_t base_vector = 0x20 + 0x20*i + j;
			for(int k = 0; k < number; ++k) {
				register_gate(base_vector + k, 1, irq[k]);
				isallocd[i] |= (1 << (j + k));
			}
			return base_vector;
		}
	}
	}
	return 0x00; // Indicates failure
}

