#include "proc.hh"
#include "kernel/kernel.hh"
#include "elf.hh"
#include "x86/asm_stubs.hh"

proc::proc(filehandler* loadfrom, filehandler* stdo, filehandler* stdi) {
	elf::file_header head;
	loadfrom->read(&head, sizeof(head));
	// Input validation
	if(strncmp((char*)(head.ident), "\177ELF", 4)) {
		throw runtime_error("Invalid ELF header");
	}

	if(head.type != elf::file_header::EXEC) {
		throw runtime_error("File is not an executable");
	}
	if(head.machine != 0x3e) {throw runtime_error("File is not an x86-64 executable");}
	if(head.obj_version != 1) {throw runtime_error("Unknown ELF version");}

	elf::program_header phead;
	auto currpos = head.ph_offset;

	context.cr3 = get_phpage();
	auto kernel_cr3 = x64::get_cr3();
	memset((x64::pml4*)(context.cr3 - 512*1024*1024*1024ul), 0, 511*sizeof(x64::hl_paging_entry)); // We do *not* do a deep copy to keep sync !
	memcpy(&(((x64::pml4*)(context.cr3 - 512*1024*1024*1024ul))->entry[511]), &(((x64::pml4*)(kernel_cr3-512*1024*1024*1024ul))->entry[511]),
	sizeof(x64::hl_paging_entry)); // Entries 510 - 511

	memset(&(((x64::pml4*)(kernel_cr3 - 512*1024*1024*1024ul))->entry[510]), 0, sizeof(x64::hl_paging_entry)); // Unmap 510 from the kernel's POV

	/*
	Map the future process' 0 - 512G range to the kernel's  -1024G -- -512G range.
	*/	

	for(unsigned i = 0; i < head.ph_entry_num; ++i) {
		//printf("Seeking to %lu\n", currpos);
		loadfrom->seek(currpos, SET);
		//printf("Reading %lu(+%lu)\n", currpos, sizeof(phead));
		loadfrom->read(&phead, sizeof(phead));
		
		if(phead.type != elf::program_header::LOAD) {
			if(phead.type == elf::program_header::DYNAMIC) {
				throw runtime_error("Dynamic loading is not yet supported");
			}
		}
		
		if(phead.vaddr >= 4*1024*1024*1024ul || phead.vaddr+phead.memsz >= 4*1024*1024*1024ul) {
			printf("Address : %lu(+%lu)\n", phead.vaddr, phead.memsz);
			throw runtime_error("Malformed ELF section");
		}

		x64::linaddr start_addr = phead.vaddr - (512*2)*1024*1024*1024ul; // Start of the 510th entry in PML4
		pphmem_man->back_vmem(start_addr, phead.memsz, 0b100); // Make page user-visible
		//printf("Seeking to %lu\n", phead.file_off);
		loadfrom->seek(phead.file_off, SET);
		//printf("Reading %lu(+%lu)\n", phead.file_off, phead.filesz);
		loadfrom->read((void*)(start_addr), phead.filesz);
		memset((uint8_t*)(start_addr) + phead.filesz, 0, phead.memsz - phead.filesz);
		
		currpos += head.ph_entry_size;
	}

	// Transport the -6 GB -- -2 GB to the process' view of the lowest 4 GiB.
	memcpy(&(((x64::pml4*)(context.cr3- 512*1024*1024*1024ul))->entry[0]), &(((x64::pml4*)(kernel_cr3- 512*1024*1024*1024ul))->entry[510]), sizeof(x64::hl_paging_entry));
	context.cs = 0x18 | 3;
	context.ss = 0x20 | 3;
	context.gp_regs[5] = (uintptr_t)(stdo); // rdi
	context.gp_regs[4] = (uintptr_t)(stdi); // rsi
	context.rip = head.entrypoint;
}; 
