#include <cstddef>
#include <cstdint>
#include "misc/display.h"
#include "interrupts.hh"
#include "x86/descriptors.hh"
#include "x86/asm_stubs.hh"
#include "pci.hh"
#include "mem/utils.hh"
#include "mem/default_pages.hh"
#include "kernel.hh"
#include "misc/memory_map.hh"
#include "drivers/serial/serial.hh"
#include "fs/rootfs.hh"
#include "kstdlib/cstring.hh"
#include "kstdlib/cstdio.hh"
#include <new>
#include <exception>
#include "kstdlib/string.hh"
#include "fs/devfs.hh"
#include "drivers/vga_text/vga_text.hh"
#include "kstdlib/cstdlib.hh"
#include "kstdlib/stdexcept.hh"

#include "fs/memf.hh"
#include "drivers/fat/fat.hh"

extern "C" void abort() {
	puts("Kernel panic !\n");
	halt();
}

extern "C" void assert(bool b, const char* c) {
	if(!b) { printf(c); halt(); }
}


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


__attribute__((interrupt)) void DE_handler(void*) {
	puts("#DE");
	halt();
	return;
}
__attribute__((interrupt)) void PF_handler(void*) {
	puts("#PF");
	halt();
	return;
}
__attribute__((interrupt)) void UD_handler(void*) {
	puts("#UD");
	halt();
	return;
}
__attribute__((interrupt)) void NP_handler(void*) {
	puts("#NP");
	halt();
	return;
}
__attribute__((interrupt)) void GP_handler(void*) {
	puts("#GP");
	halt();
	return;
}

__attribute__((interrupt)) void DF_handler(void*) {
	puts("#DF");
	halt();
	return;
}
__attribute__((interrupt)) void APIC_timer(void*) {
	puts("Bopped !\n");
	interrupt_manager imngr;
	imngr.send_EOI();
	return;
}

struct __attribute__((packed)) _gdt {
	x64::segment_descriptor null_entry;
	x64::segment_descriptor code_segment;
	x64::segment_descriptor data_segment;
	x64::system_segment_descriptor tss_desc;
};

_gdt* pgdt;
x64::TSS* ptss;
interrupt_manager* pimngr;
serial::portdriver* ser0_drv;


extern "C" void kernel_early_main(x64::linaddr istack, memory_map_entry* mmap, uint64_t kernel_zero)  {
	x64::enable_sse();
   

	static _gdt gdt; 
	static x64::TSS tss;

	pgdt = &gdt;

	gdt.tss_desc.inner.access = 0x89; // 64 bit present available TSS
	gdt.tss_desc.inner.flags = 0;
	gdt.tss_desc.set_base((x64::linaddr) (&tss));
	gdt.tss_desc.set_limit(0x67);

	gdt.code_segment = craft_code_segment();
	gdt.data_segment = craft_data_segment();
	x64::gdt_descriptor desc = {sizeof(gdt) - 1, (x64::linaddr)(&gdt)}; 

	x64::lgdt((uint64_t)(&desc));
	x64::reload_descriptors();
	x64::reload_tss(0x18);

	//uint64_t kernel_zero = 0;
	setup_heap(mmap, kernel_zero);
	mem::default_pages_init(kernel_zero);


	ser0_drv = new serial::portdriver(0x3f8); // Stable stdout
	smallptr<filehandler> fh = ser0_drv->get_file(WRONLY);
	stdout = fh.ptr;
	fh.ptr = nullptr;	

	Display().clear();

	static interrupt_manager imngr;
	pimngr = &imngr;
	
	tss.rsp[0] = istack; // For now, everything goes in istack
	for(int i = 1; i < 8; ++i) {tss.ist[i] = istack;}

	printf("Local APIC base: %p\n", imngr.apic_base());
	imngr.register_gate(0, 1, (uint64_t)(&DE_handler));
	imngr.register_gate(0x6, 1, (uint64_t)(&UD_handler));
	imngr.register_gate(0xe, 1, (uint64_t)(&PF_handler));
	imngr.register_gate(0xb, 1, (uint64_t)(&NP_handler));	
	imngr.register_gate(0xd, 1, (uint64_t)(&GP_handler));
	imngr.register_gate(0x8, 1, (uint64_t)(&DF_handler));
	x64::linaddr req_isr[1] = {(x64::linaddr)(&APIC_timer)};
	
	uint8_t vector = imngr.register_interrupt_block(1, req_isr);	
	imngr.enable();

	printf("kernel_zero was %p\n", kernel_zero);

}

extern void* _binary_fattest_raw_start;
extern void* _binary_fattest_raw_end;
extern size_t _binary_fattest_raw_size;

extern "C" void kernel_main() {	
	
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

	/*
	for(x64::linaddr d = (x64::linaddr)x; d < (x64::linaddr)(x) + 4096*1024ull; d += 4096) {
		auto phaddr = (x64::linaddr)(mem::resolve_to_phmem(d).resolved);
		if(phaddr >= (uintptr_t)(&kernel_begin) + 2048*1024*1024ull &&
		   phaddr <= (uintptr_t)(&kernel_end) + 2048*1024*1024ull) {
			printf("Found phaddr %p at vaddr %p. Seems risky !\n", phaddr, d);
		}
	}
	*/

	memset(x, 0, 4*1024*1024ull);
	puts("Still alive !");

	int* y = new int[4];
	int* z = new int[2048];
	*y = 0;
	printf("%p %p\n", y,z);

	delete[] y;
	delete[] z;

	rootfs::rootfs filesystem;
	devfs  dev_fsys;
	dev_fsys.attach_serial(smallptr<serial::portdriver>(ser0_drv));
	dev_fsys.attach_tty(smallptr<vga::text_driver>(new vga::text_driver()));
	filesystem.mnts[0].fsys = &dev_fsys;
	filesystem.mnts[0].mountpath = std::move(string("/dev"));

	auto* it = filesystem.get_iterator();

	printf("Current path %s\n", it->get_canonical_path().c_str());

	if((*it << "dev") != DIR_ENTRY) {
		printf("Filesystem failure !\n");
	}	
	printf("Current path %s\n", it->get_canonical_path().c_str());
	if((*it << "ser0") == FILE_ENTRY) {
		puts("/dev/ser0 is a file !\n");
		auto* old = stdout;
		smallptr<filehandler> fh = it->open_file("ser0", WRONLY);
		stdout = fh.ptr;
		puts("Written to /dev/ser0 :)\n");
		stdout = old;
	} else {
		puts("/dev/ser0 is not a file !");
	}

	if((*it << "tty0") == FILE_ENTRY) {
		puts("/dev/tty0 is a file !\n");
		auto* old = stdout;
		smallptr<filehandler> fh = it->open_file("tty0", WRONLY);
		stdout = fh.ptr;
		puts("Written to /dev/tty0 :)\n");
		stdout = old;
	} else {
		puts("/dev/tty0 is not a file !");
	}
	*it << ".";
	printf("Canonical path of /dev/. : %s\n", it->get_canonical_path().c_str());
	*it << "..";
	printf("Canonical path of /dev/./.. : %s\n", it->get_canonical_path().c_str());
	*it << "..";
	printf("Canonical path of /dev/./../.. : %s\n", it->get_canonical_path().c_str());


	try {
		stdout->seek(0, SET);
	} catch(einval&) {
		puts("Cannot seek on stdout !\n");
	}
	try {
		stdout->read(nullptr, 42);
	} catch(eperm&) {
		puts("Cannot read stdout !\n");
	}

	try {
		throw 'a';
	} catch(char e) {
		printf("Caught char exception e = %c\n", e);
	}
	printf("Still going !\n");
	printf("Binary object size : %p\n", &_binary_fattest_raw_size);	

	
	memfile fat_memfile(RW, (size_t)(&_binary_fattest_raw_size));
	fat_memfile.write(&_binary_fattest_raw_start, (size_t)(&_binary_fattest_raw_size));
	fat_memfile.seek(0, SET);

	FAT::filesystem fat_testfs(&fat_memfile, false);
    auto* fat_it = fat_testfs.get_iterator();
    printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    *fat_it << "fatimage";
    printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    drit_status status = (*fat_it << "longnamestresstest.txt");
    if (status == DIR_ENTRY) {
        printf("longnamestresstest.txt is a directory !\n");
    } else if (status == NP) {
        printf("longnamestresstest.txt does not exist !\n");
    } else {
        printf("longnamestresstest.txt is not a file !\n");
    }
    if(status == FILE_ENTRY) {
        char buffer[16];
        
        smallptr<filehandler> longnamestresstest = fat_it->open_file("longnamestresstest.txt", RW);
        printf("Opened longnamestresstest.txt\n");
        filehandler* fh = longnamestresstest.ptr;

        printf("Writing to longnamestresstest.txt\n");
        fh->seek(0, SET);
        printf("DEBUG 1\n");
        fh->write("Hello, world !\n", 15);
        printf("DEBUG 2\n");
        fh->seek(0, SET);
        printf("DEBUG 3\n");
        fh->read(buffer, 15);
        printf("DEBUG 4\n");
        buffer[15] = 0;
        printf("Read from longnamestresstest.txt : %s\n", buffer);

        fh->seek(0, SET);
        fh->read(buffer, 15);
        printf("Read from longnamestresstest.txt : %s\n", buffer);
    } else {
        printf("longnamestresstest.txt is not a file !\n");
    }

	/*
		TODO : Add pertinent test cases here.
	*/


	pci::enumerate(); 

	throw underflow_error("Out of cake.");
	puts("Got cake ?!\n");
	halt();
}


extern "C" void landing_pad() {
	try {
		kernel_main();
	} catch(std::exception& e) {
		puts("\n---begin kernel panic !---\n");
		puts(e.what());
		puts("\n---End kernel panic---\n");
	}
}
