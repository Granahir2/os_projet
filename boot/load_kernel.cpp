#include "load_kernel.hh"

namespace elf64 { // Move to a specific file ? We're going to need this someday elsewhere.
	struct file_header {
		unsigned char ident[16]; // Magic numbers
		enum : uint16_t {NONE,REL,EXEC,DYN,CORE,
			    LOOS,HIOS,LOPROC,HIPROC} type;
		uint16_t machine;
		uint32_t obj_version;
		
		uint64_t entrypoint;
		uint64_t ph_offset;
		uint64_t sh_offset;

		uint32_t proc_flags; 
		uint16_t ehsize; // ELF header size, for future versions
		
		uint16_t ph_entry_size; // Program headers definitions
		uint16_t ph_entry_num;

		uint16_t sh_entry_size; // Section headers definitions
		uint16_t sh_entry_num;
		uint16_t sh_strndx; // Index of the section containing the string table of the section names
	};

	struct program_header {
		enum uint32_t {UNUSED, LOAD, DYNAMIC} type; // Other types are possible, but ignored here
		uint32_t flags;
		uint64_t file_off;
		uint64_t vaddr;
		uint64_t paddr;
		uint64_t filesz;
		uint64_t memsz;
		uint64_t align;
	};
};

extern "C" uint64_t kernel_zero;

lk_result load_kernel(uint32_t safezone_start, void* elf_start) {
	elf64::file_header* fh = reinterpret_cast<decltype(fh)>(elf_start);

	if(fh->ident[0] != '\x7f' || fh->ident[1] != 'E' || fh->ident[2] != 'L' || fh->ident[3] != 'F') {
		return {lk_result::BAD_MAGIC, 0, 0};
	}

	if(fh->type != elf64::file_header::EXEC && fh->type != elf64::file_header::CORE) {
		return {lk_result::NOT_EXEC, 0, fh->type};
	}

	if(fh->machine != 0x3e) { return {lk_result::NOT_X64, 0, fh->machine};} // Not for x86-64
	// Check fh->machine / proc_flags ?
	if(fh->obj_version != 1) { return {lk_result::BAD_ELF, 0, (int)(fh->obj_version)};}
	
	const uint16_t entry_size = fh->ph_entry_size;
	const uint16_t entry_num  = fh->ph_entry_num;

	elf64::program_header* toparse = reinterpret_cast<decltype(toparse)>((uint8_t*)(elf_start) + fh->ph_offset);
	for(uint16_t i = 0; i < entry_num; ++i) {
		if(toparse->type != elf64::program_header::LOAD) {
			if(toparse->type == elf64::program_header::DYNAMIC) {
				return {lk_result::CANNOT_RELOC, 0, 0};
			} continue;
		}

		uint64_t start_addr = toparse->vaddr;
		if(start_addr >> 63) {start_addr += 2*1024*1024*1024ull;}

		start_addr += kernel_zero; /* Ensure we only load in the safe zone */
		
		if(start_addr < (uint64_t)safezone_start ||
		   start_addr + toparse->memsz >= (uint64_t)(0x100000000ull)) {
			return {lk_result::SAFEZONE_VIOLATION, 0, i};
		}

		uint8_t* memptr = (uint8_t *)(start_addr);
		uint8_t* fileptr = (uint8_t *)(elf_start) + toparse->file_off;
		for(uint64_t i = 0; i < toparse->filesz; ++i) {
			*(memptr) = *(fileptr); // Not optimized...
			memptr++; fileptr++;
		}
		for(uint64_t i = toparse->filesz; i < toparse->memsz; ++i) {
			*memptr = 0; memptr++;
		}

		toparse = reinterpret_cast<decltype(toparse)>((uint8_t*)(toparse) + entry_size);
	}
	return {lk_result::OK, (uint32_t)(fh->entrypoint & 0xffffffff), 0};
}
