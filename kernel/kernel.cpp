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
#include "mem/phmem_manager.hh"
#include "mem/utils.hh"
#include "mem/vmem_allocator.hh"
#include "kernel.hh"

extern "C" void halt();
extern "C" void  memset(void* dst,int ch,size_t sz) {
	for(size_t s = 0; s < sz; ++s) {
		((char*)dst)[s] = static_cast<unsigned char>(ch);
	}
}

extern "C" void  memcpy(void* dst,const void* src, size_t sz) {
	for(size_t s = 0; s < sz; ++s) {
		((char*)dst)[s] = ((const char*)src)[s];
	}
}


static mem::ph_tree_allocator<10> alloc;
static mem::VirtualMemoryAllocator<18> kvmem_alloc;
static mem::phmem_manager phmem_man(0);

extern "C" void* kmmap(void* hint, size_t sz) {
	if(sz == 0) return NULL;
	auto r = kvmem_alloc.mmap((x64::linaddr)(hint) + 2*1024*1024*1024ull, sz);
	if(r == mem::alloc_null) return NULL;
	phmem_man.back_vmem(r - 2*1024*1024*1024ull, sz, 0);
	return (void*)(r - 2*1024*1024*1024ull);
}

x64::phaddr get_phpage() {
	return alloc.get_page();
}

void kprintf(const char* str, ...) {	
	va_list args;
	va_start(args, str);
	Display().vprint(str, args);
	va_end(args);
}

void test_phmem_resolving(x64::linaddr l) {
	auto z = mem::resolve_to_phmem(l);

	kprintf("Resolving %p to %p\n",
		l, z.resolved);
	if(z.status == mem::phmem_resolvant::NOT_CANONICAL) {
		kprintf("Because the address wasn't canonical\n");
	} else if(z.status == mem::phmem_resolvant::NOT_MAPPED) {
		kprintf("Because the address wasn't mapped\n");
	}
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
	test_phmem_resolving((x64::linaddr)(-1));
	test_phmem_resolving(0);
	test_phmem_resolving((1ull << 32));

	test_phmem_resolving((x64::linaddr)(-4096));
	mem::phmem_manager phmemm(0);
	phmemm.back_vmem((x64::linaddr)(-2*4096), 2*4096, 0);

	test_phmem_resolving((x64::linaddr)(-4096));
	test_phmem_resolving((x64::linaddr)(-2*4096));*/


	kvmem_alloc = mem::VirtualMemoryAllocator<18>();
	kprintf("Requested 2 pages, got : %p\n", kvmem_alloc.mmap(0, 8192));
	kprintf("Requested 1 page,  got : %p\n", kvmem_alloc.mmap(0, 4096));
	kprintf("Freed 4096 - 8191\n");
	kvmem_alloc.munmap(4096, 4096);
	kprintf("Freed 0 - 4095\n");
	kvmem_alloc.munmap(0, 4096);
	kprintf("Requested 1 page, got : %p\n", kvmem_alloc.mmap(0, 4096));	
	kprintf("Requested 2 pages, got : %p\n", kvmem_alloc.mmap(0, 8192));

	kprintf("Tried to mmap 1 page : %p", kmmap(NULL, 4096));
	while(true) {}
}
