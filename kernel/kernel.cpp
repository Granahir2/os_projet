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
#include "mem/default_pages.hh"
#include "kernel.hh"
#include "misc/memory_map.hh"
#include "drivers/serial/serial.hh"

#include "kstdlib/cstring.hh"
#include "kstdlib/cstdio.hh"

static mem::ph_tree_allocator<10> alloc;
static mem::VirtualMemoryAllocator<18> kvmem_alloc;
static mem::phmem_manager phmem_man(0);

unsigned long long min(unsigned long long a, unsigned long long b) {
	if( a < b) {return a;} else {return b;}
}

extern "C" void* kmmap(void* hint, size_t sz) {
	if(sz == 0) return NULL;
	auto r = kvmem_alloc.mmap((x64::linaddr)(hint) + 2*1024*1024*1024ull, sz);
	if(r == mem::alloc_null) return NULL;
	phmem_man.back_vmem(r - 2*1024*1024*1024ull, sz, 0);
	return (void*)(r - 2*1024*1024*1024ull);
}

extern "C" void kmunmap(void* ptr, size_t sz) {
	if(sz == 0) return;
	kvmem_alloc.munmap((x64::linaddr)ptr + 2*1024*1024*1024ull, sz);
	phmem_man.unback_vmem((x64::linaddr)ptr, sz, 0);
}

x64::phaddr get_phpage() {auto x = alloc.get_page(); return x;}
void release_phpage(x64::phaddr p) {alloc.release_page(p);}

void test_phmem_resolving(x64::linaddr l) {
	auto z = mem::resolve_to_phmem(l);

	printf("Resolving %p to %p ",
		l, z.resolved);
	if(z.status == mem::phmem_resolvant::NOT_CANONICAL) {
		puts("Because the address wasn't canonical\n");
		return;
	} else if(z.status == mem::phmem_resolvant::NOT_MAPPED) {
		puts("Because the address wasn't mapped\n");
		return;
	}
	if(z.isRW) {
		puts("RW\n");
	} else {puts("RO\n");}
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
	printf("#DE");
	halt();
}
__attribute__((interrupt)) void PF_handler(void*) {
	printf("#PF");
	halt();
}
__attribute__((interrupt)) void GP_handler(void*) {
	printf("#GP");
	halt();
}

__attribute__((interrupt)) void DF_handler(void*) {
	printf("#DF");
	halt();
}
__attribute__((interrupt)) void APIC_timer(void*) {
	printf("Bopped !\n");
	interrupt_manager imngr;
	imngr.send_EOI();	
}

// Linker defined symbols
extern "C" int kernel_begin;
extern "C" int kernel_end;

x64::gdt_descriptor desc;
extern "C" void kernel_main(x64::linaddr istack, memory_map_entry* mmap)  {
    	mem::default_pages_init();
	Display().clear();

	serial::portdriver com(0x3f8);
	stdout = &com;

	tss.rsp[0] = istack; // For now, everything goes in istack
	for(int i = 1; i < 8; ++i) {tss.ist[i] = istack;}


	kvmem_alloc = mem::VirtualMemoryAllocator<18>();
	kvmem_alloc.mark_used((x64::linaddr)(&kernel_begin) + 2*1024*1024*1024ull,
			(x64::linaddr)(&kernel_end) - (x64::linaddr)(&kernel_begin));

	printf("Kernel begin:end %p:%p",  (x64::linaddr)(&kernel_begin) + 2*1024*1024*1024ull,
			(x64::linaddr)(&kernel_end) + 2*1024*1024*1024ull);

	alloc = decltype(alloc)();
	alloc.mark_used((x64::linaddr)(&kernel_begin) + 2*1024*1024*1024ull,
			(x64::linaddr)(&kernel_end) + 2*1024*1024*1024ull);

	printf("Memory map at %p\n", mmap);
	int i = 0;
	for(; mmap[i].length != 0; ++i) {
		printf("%p:%p ", mmap[i].base, mmap[i].base + mmap[i].length - 1, mmap[i].type);
		switch(mmap[i].type) {
			case 1:
				puts("available\n"); break;
			case 3:
				puts("acpi\n"); break;
			case 4:
				puts("hib. safe\n"); break;
			case 5:
				puts("bad ram\n"); break;
			default:
				puts("reserved\n");
		}
		
		if(mmap[i].type != 1 && mmap[i].base < 0x100000000ull) {
			auto end = (mmap[i].base + mmap[i].length - 1);
			if(end >= 0x100000000ull) {end = 0xffffffff;}
			alloc.mark_used(mmap[i].base, end);
		}

		if(i > 0 && mmap[i - 1].base + mmap[i - 1].length < mmap[i].base
			&& mmap[i - 1].base + mmap[i - 1].length < 0x100000000ull) {
			alloc.mark_used(mmap[i - 1].base + mmap[i - 1].length, min(mmap[i].base - 1, (uint64_t)0xffffffffull));
		}
	}
	if(mmap[i-1].base + mmap[i-1].length < 0x100000000ull) {
		printf("Filling up %p %p\n", mmap[i-1].base + mmap[i-1].length, 0xffffffffull);
		alloc.mark_used(mmap[i-1].base + mmap[i-1].length, 0xffffffffull);
	}

	printf("Alloc status : %d %d %d %d\n", alloc.children[0].isfull(),
					     alloc.children[1].isfull(),
					     alloc.children[2].isfull(),
					     alloc.children[3].isfull());

	gdt.tss_desc.inner.access = 0x89; // 64 bit present available TSS
	gdt.tss_desc.inner.flags = 0;
	gdt.tss_desc.set_base((x64::linaddr) (&tss));
	gdt.tss_desc.set_limit(0x67);

	gdt.code_segment = craft_code_segment();
	gdt.data_segment = craft_data_segment();
	desc = {sizeof(gdt) - 1, (x64::linaddr)(&gdt)}; 

	x64::lgdt((uint64_t)(&desc));
	x64::reload_descriptors();
	x64::reload_tss(0x18);

	interrupt_manager imngr;
	printf("Local APIC base: %p\n", imngr.apic_base());
	imngr.register_gate(0, 1, (uint64_t)(&DE_handler));
	imngr.register_gate(0xe, 1, (uint64_t)(&PF_handler));
	imngr.register_gate(0xd, 1, (uint64_t)(&GP_handler));
	imngr.register_gate(0x8, 1, (uint64_t)(&DF_handler));
	x64::linaddr req_isr[1] = {(x64::linaddr)(&APIC_timer)};
	
	uint8_t vector = imngr.register_interrupt_block(1, req_isr);	
	imngr.enable();
	
	auto x = kmmap((void*)(-2*1024*1024*1024ull), 4096);
	printf("Tried to mmap 1 page : %p\n", x);
	test_phmem_resolving(x64::linaddr(x));

	x = kmmap((void*)(-2*1024*1024*1024ull), 4096);
	printf("Tried to mmap 1 page : %p\n", x);
	test_phmem_resolving(x64::linaddr(x));
	
	x = kmmap(NULL, 8192);
	printf("Tried to mmap 2 pages : %p\n", x);
	test_phmem_resolving(x64::linaddr(x));

	kmunmap(x, 4096);
	printf("Freed first page\n");
	test_phmem_resolving(x64::linaddr(x));
	test_phmem_resolving(x64::linaddr(x) + 4096);

	x = kmmap(NULL, 4*1024*1024);
	printf("Allocating 4 MiB : %p\n", x);
	for(int i = 0; i < 1024; ++i) {
		test_phmem_resolving(x64::linaddr(x) + i*4096);
	}
	
	memset(x, 0, 4*1024*1024ull);
	puts("Still alive !");
	while(true) {
		if(int s = stdout->read(x, 4*1024*1024ull)) {
			puts("Received a nice message !");
		}
	}
	halt();
}
