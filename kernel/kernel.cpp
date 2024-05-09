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

#include "drivers/ahci/ahci.hh"
#include "drivers/ahci/interface.hh"
#include "drivers/pci/configspace.hh"

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

/*
extern void* _binary_fattest_raw_start;
extern void* _binary_fattest_raw_end;
extern size_t _binary_fattest_raw_size;
*/
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

	pci::enumerate(); 
	pci::cspace conf_space;
	smallptr<filehandler> f = conf_space.get_file(0b100000, RW);
	ahci::driver drv(f.ptr);
	auto nf = ahci::file(RW, 0, &drv);

	unsigned char lol[512];
	nf.read(lol, 512);
	printf("Boot sector of drive :\n");
	for(int i = 0; i < 512; ++i) {
		auto c = (lol[i] >= 32 && lol[i] < 128) ? lol[i] : '.'; 
		if(i % 16 == 15) {
			printf("%c\n",c);
		} else {
			printf("%c",c);
		}
	} 
	nf.seek(0, SET);

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
    puts("Looked up longnamestresstest.txt\n"); // If lines 299 to 301 are commented out (no desync), we get further along the tests (pass TEST 1) but eventually fail for similar reasons
    if (status == DIR_ENTRY) {
        printf("longnamestresstest.txt is a directory !\n");
    } else if (status == NP) {
        printf("longnamestresstest.txt does not exist !\n");
    } else if(status == FILE_ENTRY) {
        char buffer[16];
        
        smallptr<filehandler> longnamestresstest = fat_it->open_file("longnamestresstest.txt", RW);
        filehandler* fh = longnamestresstest.ptr;

        // Try to read empty file
        fh->seek(0, SET);
        read_number_of_bytes = fh->read(buffer, 15);
        if (read_number_of_bytes != 0) {
            printf("Error while reading from empty file: longnamestresstest.txt\n");
        }

        // Try to write to empty file
        //printf("Writing to longnamestresstest.txt\n");
        fh->seek(0, SET);
        written_number_of_bytes = fh->write("Hello, world !\n", 15);
        if (written_number_of_bytes != 15) {
            printf("Error while writing to longnamestresstest.txt\n");
        }
        fh->seek(0, SET);
        read_number_of_bytes = fh->read(buffer, 15);
        if (read_number_of_bytes != 15) {
            printf("Error while reading from longnamestresstest.txt\n");
        }
    } else {
        printf("longnamestresstest.txt is not a file !\n");
    }
    printf("File system : TEST 1 PASSED\n");
    // Stress test with long directory names
    *fat_it << "superduperultramegalongdirectoryname";
    //printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    status = *fat_it << "anothersuperduperultramegalongfilename.txt";
    if (status == DIR_ENTRY) {
        printf("anothersuperduperultramegalongfilename.txt is a directory !\n");
    } else if (status == NP) {
        printf("anothersuperduperultramegalongfilename.txt does not exist !\n");
    } else if(status == FILE_ENTRY) {
        char buffer[16];
        
        smallptr<filehandler> anothersuperduperultramegalongfilename = fat_it->open_file("anothersuperduperultramegalongfilename.txt", RW);
        filehandler* fh = anothersuperduperultramegalongfilename.ptr;
        
        fh->seek(0, SET);
        size_t written_number_of_bytes = fh->write("Hello, world !\n", 15);
        if (written_number_of_bytes != 15) {
            printf("Error while writing to anothersuperduperultramegalongfilename.txt\n");
        }
        fh->seek(0, SET);
        size_t read_number_of_bytes = fh->read(buffer, 15);
        if (read_number_of_bytes != 15) {
            printf("Error while reading from anothersuperduperultramegalongfilename.txt\n");
        }
    } else {
        printf("anothersuperduperultramegalongfilename.txt is not a file !\n");
    }
    printf("File system : TEST 2 PASSED\n");

    // Test going back to parent directory
    *fat_it << "..";
    //printf("Current path : %s\n", fat_it->get_canonical_path().c_str());
    status = (*fat_it << "toto.txt");
    if (status == DIR_ENTRY) {
        printf("toto.txt is a directory !\n");
    } else if (status == NP) {
        printf("toto.txt does not exist !\n");
    } else if(status == FILE_ENTRY) {
        char buffer[16];
        
        smallptr<filehandler> longnamestresstest = fat_it->open_file("toto.txt", RW);
        filehandler* fh = longnamestresstest.ptr;

        fh->seek(0, SET);
        fh->read(buffer, 15);
        char supposed_buffer[14] = "Here be text\r";
        if (strncmp(buffer, supposed_buffer, 13) != 0) 
        {
            printf("Error while reading from toto.txt\n");
            printf("Supposed buffer : ");
            for (int i = 0; i < 13; i++) printf("%d ", supposed_buffer[i]);
            printf("\n");
            printf("Buffer : ");
            for (int i = 0; i < 13; i++) printf("%d ", buffer[i]);
            printf("\n");
            printf("Comparison result : %d\n", strcmp(buffer, supposed_buffer));
        }
    } else {
        printf("toto.txt is not a file !\n");
    }
    printf("File system : TEST 3 PASSED\n");

    // Test opening and reading a freshly written file that was empty before
    *fat_it << "longnamestresstest.txt";
    smallptr<filehandler> longnamestresstest = fat_it->open_file("longnamestresstest.txt", RW);
    filehandler* fh = longnamestresstest.ptr;
    char buffer[16];
    fh->seek(0, SET);
    read_number_of_bytes = fh->read(buffer, 15);
    if (read_number_of_bytes != 15) {
        printf("Error while reading from longnamestresstest.txt\n");
    }
    printf("File system : TEST 4 PASSED\n");

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
