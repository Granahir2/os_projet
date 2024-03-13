// From https://wiki.osdev.org/Bare_Bones, used with discretion
// Placeholder 64 bit kernel

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "misc/display.h"
#include "interrupts.hh"
#include "x86/descriptors.hh"
#include "x86/asm_stubs.hh"
#include "pci.hh"
#include "mem/phmem_allocator.hh"

extern "C" void halt();

void kprintf(const char* str, ...) {	
	va_list args;
	va_start(args, str);
	Display().vprint(str, args);
	va_end(args);
}

x64::TSS tss;

constexpr x64::segment_descriptor craft_code_segment() {
	x64::segment_descriptor retval;
	retval.access = 0b10011011; // RX segment, accessible only to Ring 0 
	retval.set_limit(0x0fffff);
	retval.flags = 0b1010; // 64-bit mode page-granularity
	return retval;
};

constexpr x64::segment_descriptor craft_data_segment() {
	x64::segment_descriptor retval;
	retval.access = 0b10010011; // RW segment, accessible only to Ring 0
	retval.set_limit(0x0fffff);
	retval.flags = 0b1100;
	return retval;
};

struct __attribute__((packed)) {
	x64::segment_descriptor null_entry;
	x64::segment_descriptor code_segment;
	x64::segment_descriptor data_segment;
	x64::system_segment_descriptor tss_desc;
} gdt;

__attribute__((interrupt)) void DE_handler(void*) {
	kprintf("#DE");
	halt();
}
__attribute__((interrupt)) void PF_handler(void*) {
	kprintf("#PF");
	halt();
}
__attribute__((interrupt)) void GP_handler(void*) {
	kprintf("#GP");
	halt();
}

__attribute__((interrupt)) void APIC_timer(void*) {
	kprintf("Bopped !\n");
	interrupt_manager imngr;
	imngr.send_EOI();	
}

x64::gdt_descriptor desc;
extern "C" void kernel_main(x64::linaddr istack)  {
    	Display().clear();
	kprintf("Hello, %d bit kernel World!\n", 64);
	kprintf("Interrupt stack at : %p \n", istack);
	tss.rsp[0] = istack; // For now, everything goes in istack
	for(int i = 1; i < 8; ++i) {tss.ist[i] = istack;}

	gdt.tss_desc.inner.access = 0x89; // 64 bit present available TSS
	gdt.tss_desc.inner.flags = 0;
	gdt.tss_desc.set_base((x64::linaddr) (&tss));
	gdt.tss_desc.set_limit(0x67);

	gdt.code_segment = craft_code_segment();
	gdt.data_segment = craft_data_segment();
	desc = {sizeof(gdt) - 1, (x64::linaddr)(&gdt)}; 

	x64::lgdt((uint64_t)(&desc));
	kprintf("GDT loaded !\n");
	x64::reload_descriptors();
	kprintf("Segment selectors loaded !\n");
	x64::reload_tss(0x18);

	interrupt_manager imngr;
	kprintf("Local APIC base : %p\n", imngr.apic_base());
	imngr.register_gate(0, 1, (uint64_t)(&DE_handler));
	imngr.register_gate(0xe, 1, (uint64_t)(&PF_handler));
	imngr.register_gate(0xd, 1, (uint64_t)(&GP_handler));

	x64::linaddr req_isr[1] = {(x64::linaddr)(&APIC_timer)};
	
	uint8_t vector = imngr.register_interrupt_block(1, req_isr);
	kprintf("Got vector number : %d\n", (int)vector);
	
	imngr.enable();
	kprintf("We chose to enable interrupts, not because it is easy, but because it is hard.\n");	
	kprintf("Local APIC ID : %d version : %x\n", imngr.apic_id(), imngr.apic_version());
	kprintf("IO APIC version : %x\n", imngr.ioapic_version());

	/*
	mem::ph_tree_allocator<10> phalloc;
	kprintf("Allocated 1 GiB of physical memory\n");
	int x = phalloc.get_page();
	int y = phalloc.get_page();
	phalloc.release_page(x);
	int z = phalloc.get_page();
	kprintf("Got physical pages %d %d %d %d\n", x, y, z,phalloc.get_page());

	*(uint32_t*)(4) = 0xdeadbeef;
	*/
	auto apic = (uint32_t volatile*)imngr.apic_base();
	apic[0x3e0/4] = 0x3; // div 16
	apic[0x320/4] = vector | 0x20000;
	apic[0x380/4] = 0x1000000;
	//pci::enumerate();
	/*int u = 0;
	int v = 3;
	v = 3/0;*/
	while(true) {}
}
