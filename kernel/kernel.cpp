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
#include "proc/proc.hh"
#include "drivers/ahci/ahci.hh"
#include "drivers/ahci/interface.hh"
#include "drivers/pci/configspace.hh"

#include "kernel/proc/scheduler_main.hh"

extern "C" void abort() {
	puts("Irrecoverable kernel panic !");
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
		puts("Because the address wasn't canonical");
		return;
	} else if(z.status == mem::phmem_resolvant::NOT_MAPPED) {
		puts("Because the address wasn't mapped");
		return;
	}
	if(z.isRW) {
		puts("RW");
	} else {puts("RO");}
}

constexpr x64::segment_descriptor craft_code_segment(unsigned dpl) {
	x64::segment_descriptor retval;
	retval.access = 0b10011011 | ((dpl & 0b11) << 5); // RX segment, accessible only to Ring 0 
	retval.set_limit(0x0fffff);
	retval.flags = 0b1010; // 64-bit mode page-granularity
	return retval;
};

constexpr x64::segment_descriptor craft_data_segment(unsigned dpl) {
	x64::segment_descriptor retval;
	retval.access = 0b10010011 | ((dpl & 0b11) << 5); // RW segment, accessible only to Ring 0
	retval.set_limit(0x0fffff);
	retval.flags = 0b1100;
	return retval;
};



extern "C" {
void saveregs_hook();
void loadregs_hook();
}

extern "C" void print_regs() {
	/*
	for(int i = 0; i < 16; ++i) {
		printf("REG[%d] = %lx\n", i, registers.gp_regs[i]);
	}

	printf("RIP = %lx RFLAGS = %lx\n", registers.rip, registers.flags);
	printf("SS = %lx CS = %lx\n", registers.ss, registers.cs);*/
}


__attribute__((interrupt)) void DE_handler(interrupt_frame* ifr) {
	puts("#DE");
	printf("RIP : %lx\n", ifr->rip);
	printf("CS : %lx\n", ifr->cs);
	printf("RFLAGS : %lx\n", ifr->rflags);
	printf("RSP : %lx\n", ifr->rsp);
	printf("SS : %lx\n", ifr->ss);

	halt();
	return;
}
__attribute__((interrupt)) void PF_handler(interrupt_frame* ifr, size_t code) {
	printf("#PF(%x)\n", (int)code);
	printf("CR2 : %lx\n", x64::get_cr2());

	printf("RIP : %lx\n", ifr->rip);
	printf("CS : %lx\n", ifr->cs);
	printf("RFLAGS : %lx\n", ifr->rflags);
	printf("RSP : %lx\n", ifr->rsp);
	printf("SS : %lx\n", ifr->ss);

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
__attribute__((interrupt)) void GP_handler(interrupt_frame* ifr, size_t code) {
	printf("#GP(%x)\n", (int)code);
	printf("RIP : %lx\n", ifr->rip);
	printf("CS : %x\n", (int)ifr->cs);
	printf("RFLAGS : %lx\n", ifr->rflags);
	printf("RSP : %lx\n", ifr->rsp);
	printf("SS : %x\n", (int)ifr->ss);
	halt();
	return;
}

__attribute__((interrupt)) void DF_handler(void*) {
	puts("#DF");
	halt();
	return;
}
__attribute__((interrupt)) void APIC_timer(void*) {
	puts("Bopped !");
	interrupt_manager imngr;
	imngr.send_EOI();
	return;
}

_gdt* pgdt;
x64::TSS* ptss;
interrupt_manager* pimngr;
serial::portdriver* ser0_drv;

extern "C" void kernel_early_main(x64::linaddr istack, memory_map_entry* mmap, uint64_t kernel_zero)  {
	x64::enable_sse();
	x64::enable_xsave();   

	static _gdt gdt; 
	static x64::TSS tss;

	pgdt = &gdt;
	ptss = &tss;

	gdt.tss_desc.inner.access = 0x89; // 64 bit present available TSS
	gdt.tss_desc.inner.flags = 0;
	gdt.tss_desc.set_base((x64::linaddr) (&tss));
	gdt.tss_desc.set_limit(0x67);

	gdt.code_segment = craft_code_segment(0);
	gdt.data_segment = craft_data_segment(0);
	gdt.code_segment_ring3 = craft_code_segment(3);
	gdt.data_segment_ring3 = craft_data_segment(3);
	x64::gdt_descriptor desc = {sizeof(gdt) - 1, (x64::linaddr)(&gdt)}; 

	x64::lgdt((uint64_t)(&desc));
	x64::reload_descriptors();
	x64::reload_tss((intptr_t)(&gdt.tss_desc) - (intptr_t)(&gdt));

	//uint64_t kernel_zero = 0;
	setup_heap(mmap, kernel_zero);
	mem::default_pages_init(kernel_zero);

	
	ser0_drv = new serial::portdriver(0x3f8); // Stable stdout
	smallptr<filehandler> fh = ser0_drv->get_file(WRONLY);
	stdout = fh.ptr;
	fh.ptr = nullptr;	

	static interrupt_manager imngr;
	pimngr = &imngr;
	
	tss.rsp[0] = istack; // For now, everything goes in istack
	tss.ist[1] = istack;
	for(int i = 2; i < 8; ++i) {tss.ist[i] = istack+8096;}

	printf("Local APIC base: %p\n", imngr.apic_base());
	imngr.register_gate(0, 1, (uint64_t)(&DE_handler));
	imngr.register_gate(0x6, 1, (uint64_t)(&UD_handler));
	imngr.register_gate(0xe, 1, (uint64_t)(&PF_handler));
	imngr.register_gate(0xb, 1, (uint64_t)(&NP_handler));	
	imngr.register_gate(0xd, 1, (uint64_t)(&GP_handler));
	imngr.register_gate(0x8, 1, (uint64_t)(&DF_handler));

	imngr.register_gate(32, 2, (uint64_t)(&saveregs_hook), 3); // Scheduler interrupt
	imngr.register_gate(33, 3, (uint64_t)(&syscall_hook), 3); // Syscall interrupt

	x64::linaddr req_isr[1] = {(x64::linaddr)(&saveregs_hook)};
	uint8_t base = imngr.register_interrupt_block(1, &req_isr[0]);
	printf("Got %d\n", (int)base);
	imngr.apic_base()[0x320/4] = 0x00010000 | base; // Setup APIC on "base" interrupt;
	// don't trigger it yet.

	imngr.enable();

	printf("kernel_zero was %p\n", kernel_zero);

}

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
	auto td = vga::text_driver();
	td.clear();
	dev_fsys.attach_tty(smallptr<vga::text_driver>(new vga::text_driver()));
	filesystem.mnts[0].fsys = &dev_fsys;
	filesystem.mnts[0].mountpath = std::move(string("/dev"));

	auto* it = filesystem.get_iterator();

	printf("Current path %s\n", it->get_canonical_path().c_str());

	if((*it << "dev") != DIR_ENTRY) {
		puts("Filesystem failure !");
	}	
	printf("Current path %s\n", it->get_canonical_path().c_str());
	if((*it << "ser0") == FILE_ENTRY) {
		puts("/dev/ser0 is a file !");
		auto* old = stdout;
		smallptr<filehandler> fh = it->open_file("ser0", WRONLY);
		stdout = fh.ptr;
		puts("Written to /dev/ser0 :)");
		stdout = old;
	} else {
		puts("/dev/ser0 is not a file !");
	}

	if((*it << "tty0") == FILE_ENTRY) {
		puts("/dev/tty0 is a file !");
		auto* old = stdout;
		smallptr<filehandler> fh = it->open_file("tty0", WRONLY);
		stdout = fh.ptr;
		puts("Written to /dev/tty0 :)");
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
		puts("Cannot seek on stdout !");
	}
	try {
		stdout->read(nullptr, 42);
	} catch(eperm&) {
		puts("Cannot read stdout !");
	}

	pci::enumerate(); 
	pci::cspace conf_space;
	smallptr<filehandler> f = conf_space.get_file(0b100000, RW);
	ahci::driver drv(f.ptr);
	auto nf = ahci::file(RW, 0, &drv);

	unsigned char lol[512];
	puts("Before read");
	nf.read(lol, 512);
	puts("Boot sector of drive :");
	for(int i = 0; i < 512; ++i) {
		auto c = (lol[i] >= 32 && lol[i] < 128) ? lol[i] : '.'; 
		if(i % 16 == 15) {
			printf("%c\n",c);
		} else {
			putchar(c);
		}
	} 
	nf.seek(0, SET);
	strcpy((char*)lol, "Trash! Trash everywhere in this drive!"); 
	nf.seek(0, SET);
	fputs("Mdr lol", &nf);
	nf.seek(0, SET);
	nf.read(lol, 512);
	nf.seek(0, SET);
	puts("Boot sector of drive :");
	for(int i = 0; i < 512; ++i) {
		auto c = (lol[i] >= 32 && lol[i] < 128) ? lol[i] : '.'; 
		if(i % 16 == 15) {
			printf("%c\n",c);
		} else {
			putchar(c);
		}
	} 


	FAT::filesystem fat_testfs(&nf, true);
    size_t read_number_of_bytes, written_number_of_bytes;
    auto* fat_it = fat_testfs.get_iterator();
    //printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    *fat_it << "fatimage";
    //printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    *fat_it << "superduperultramegalongdirectoryname"; // BUG : we don't detect this as a directory
    printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    *fat_it << "..";
    printf("Current path : %s\n", fat_it->get_canonical_path().c_str()); // We end up at the root
    drit_status status = (*fat_it << "longnamestresstest.txt"); // This directory lookup *fails* by seeking out of the filesystem entirely
    puts("Looked up longnamestresstest.txt"); // If lines 299 to 301 are commented out (no desync), we get further along the tests (pass TEST 1) but eventually fail for similar reasons
    if (status == DIR_ENTRY) {
        puts("longnamestresstest.txt is a directory !");
    } else if (status == NP) {
        puts("longnamestresstest.txt does not exist !");
    } else if(status == FILE_ENTRY) {
        char buffer[16];
        
        smallptr<filehandler> longnamestresstest = fat_it->open_file("longnamestresstest.txt", RW);
        filehandler* fh = longnamestresstest.ptr;

        // Try to read empty file
        fh->seek(0, SET);
        read_number_of_bytes = fh->read(buffer, 15);
        if (read_number_of_bytes != 0) {
            puts("Error while reading from empty file: longnamestresstest.txt");
        }

        // Try to write to empty file
        //printf("Writing to longnamestresstest.txt\n");
        fh->seek(0, SET);
        written_number_of_bytes = fh->write("Hello, world !\n", 15);
        if (written_number_of_bytes != 15) {
            puts("Error while writing to longnamestresstest.txt");
        }
        fh->seek(0, SET);
        read_number_of_bytes = fh->read(buffer, 15);
        if (read_number_of_bytes != 15) {
            puts("Error while reading from longnamestresstest.txt");
        }
    } else {
        puts("longnamestresstest.txt is not a file !");
    }
    puts("File system : TEST 1 PASSED");
    // Stress test with long directory names
    *fat_it << "superduperultramegalongdirectoryname";
    //printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    status = *fat_it << "anothersuperduperultramegalongfilename.txt";
    if (status == DIR_ENTRY) {
        puts("anothersuperduperultramegalongfilename.txt is a directory !");
    } else if (status == NP) {
        puts("anothersuperduperultramegalongfilename.txt does not exist !");
    } else if(status == FILE_ENTRY) {
        char buffer[16];
        
        smallptr<filehandler> anothersuperduperultramegalongfilename = fat_it->open_file("anothersuperduperultramegalongfilename.txt", RW);
        filehandler* fh = anothersuperduperultramegalongfilename.ptr;
        
        fh->seek(0, SET);
        size_t written_number_of_bytes = fh->write("Hello, world !\n", 15);
        if (written_number_of_bytes != 15) {
            puts("Error while writing to anothersuperduperultramegalongfilename.txt");
        }
        fh->seek(0, SET);
        size_t read_number_of_bytes = fh->read(buffer, 15);
        if (read_number_of_bytes != 15) {
            puts("Error while reading from anothersuperduperultramegalongfilename.txt");
        }
    } else {
        puts("anothersuperduperultramegalongfilename.txt is not a file !");
    }
    puts("File system : TEST 2 PASSED");

    // Test going back to parent directory
    *fat_it << "..";
    //printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    status = (*fat_it << "toto.txt");
    if (status == DIR_ENTRY) {
        puts("toto.txt is a directory !");
    } else if (status == NP) {
        puts("toto.txt does not exist !");
    } else if(status == FILE_ENTRY) {
        char buffer[16];
        
        smallptr<filehandler> longnamestresstest = fat_it->open_file("toto.txt", RW);
        filehandler* fh = longnamestresstest.ptr;

        fh->seek(0, SET);
        fh->read(buffer, 15);
        char supposed_buffer[14] = "Here be text\n";
        if (strncmp(buffer, supposed_buffer, 13) != 0) 
        {
            puts("Error while reading from toto.txt");
            fputs("Supposed buffer : ", stdout);
            for (int i = 0; i < 13; i++) printf("%d ", supposed_buffer[i]);
            puts("");
            printf("Buffer : ");
            for (int i = 0; i < 13; i++) printf("%d ", buffer[i]);
            puts("");
            printf("Comparison result : %d\n", strcmp(buffer, supposed_buffer));
        }
    } else {
        puts("toto.txt is not a file !");
    }
    puts("File system : TEST 3 PASSED");

    // Test opening and reading a freshly written file that was empty before
    *fat_it << "longnamestresstest.txt";
    smallptr<filehandler> longnamestresstest = fat_it->open_file("longnamestresstest.txt", RW);
    filehandler* fh = longnamestresstest.ptr;
    char buffer[16];
    fh->seek(0, SET);
    read_number_of_bytes = fh->read(buffer, 15);
    if (read_number_of_bytes != 15) {
        puts("Error while reading from longnamestresstest.txt");
    }
    puts("File system : TEST 4 PASSED");

	smallptr<filehandler> objfile = fat_it->open_file("test.elf", RW);
	proc process(objfile.ptr, stdout);	
	init[0] = &process;

	objfile.ptr->seek(0, SET);
	proc process2(objfile.ptr, stdout);	
	init[1] = &process2;
	objfile.ptr->seek(0, SET);
	proc process3(objfile.ptr, stdout);	
	init[2] = &process3;

	puts("Launching userspace soon (tm)");

	pimngr->apic_base()[0x320/4] &= ~(1ul << 16);// Setup APIC on "base" interrupt;
	pimngr->apic_base()[0x3e0/4] = 0;
	pimngr->apic_base()[0x380/4] = 1 << 28;
	while(true) {}	
	puts("Should have become unreacheable.");
	halt();
}


extern "C" void landing_pad() {
	try {
		kernel_main();
	} catch(std::exception& e) {
		puts("\n---begin kernel panic !---");
		puts(e.what());
		puts("---End kernel panic---");
	}
}
