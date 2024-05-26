#include "proc.hh"
#include "kernel/kernel.hh"
#include "elf.hh"
#include "x86/asm_stubs.hh"
#include "kernel/mem/utils.hh"
size_t proc::c_pid = 0; 

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
	sizeof(x64::hl_paging_entry)); // Entry 511

	x64::load_cr3(context.cr3);

	/*
	Map the future process' 0 - 512G range to the kernel's  -1024G -- -512G range.
	*/	

	//printf("ELF has %d program header entries\n", head.ph_entry_num);
	for(unsigned i = 0; i < head.ph_entry_num; ++i) {
		loadfrom->seek(currpos, SET);
		loadfrom->read(&phead, sizeof(phead));
		
		if(phead.type != elf::program_header::LOAD) {
			//printf("currpos = %lx\n", currpos);
			//printf("Skipping entry %d of type %lx\n", i, phead.type);
			if(phead.type == elf::program_header::DYNAMIC) {
				throw runtime_error("Dynamic loading is not yet supported");
			}
		}
		
		if(phead.vaddr >= 4*1024*1024*1024ul || phead.vaddr+phead.memsz >= 4*1024*1024*1024ul) {
			throw runtime_error("Malformed ELF section");
		}

		x64::linaddr start_addr = phead.vaddr;
		pphmem_man->back_vmem(start_addr, phead.memsz, 0b100); // Make page user-visible
		x64::load_cr3(context.cr3);
		loadfrom->seek(phead.file_off, SET);
		size_t s = loadfrom->read((void*)(start_addr), phead.filesz);
		//printf("Loaded %lu bytes to %p from offset %lx\n", phead.filesz, start_addr, phead.file_off);
		if(s != phead.filesz) {throw runtime_error("Could not read whole section");}
		memset((uint8_t*)(start_addr) + phead.filesz, 0, phead.memsz - phead.filesz);
		currpos += head.ph_entry_size;
	}
	
	context.flags |= 1 << 9; // Enable interrupts !
	context.cs = 0x18 | 3;
	context.ss = 0x20 | 3;
	context.gp_regs[5] = (uintptr_t)(stdo); // rdi
	context.gp_regs[4] = (uintptr_t)(stdi); // rsi
	context.rip = head.entrypoint;

	pid = c_pid++;	
};

size_t proc::get_pid() {return pid;}
