#include "load_kernel.hh"


extern "C" void terminal_printhex(uint32_t d);
void terminal_writestring(const char* data);

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

lk_result load_kernel(uint32_t safezone_start, void* elf_start) {
	elf64::file_header* fh = reinterpret_cast<decltype(fh)>(elf_start);

	if(fh->ident[0] != '\x7f' || fh->ident[1] != 'E' || fh->ident[2] != 'L' || fh->ident[3] != 'F') {
		return {lk_result::BAD_MAGIC, nullptr, 0};
	}

	if(fh->type != elf64::file_header::EXEC && fh->type != elf64::file_header::CORE) {
		return {lk_result::NOT_EXEC, nullptr, fh->type};
	}

	if(fh->machine != 0x3e) { return {lk_result::NOT_X64, nullptr, fh->machine};} // Not for x86-64
	// Check fh->machine / proc_flags ?
	if(fh->obj_version != 1) { return {lk_result::BAD_ELF, nullptr, (int)(fh->obj_version)};}
	
	const uint16_t entry_size = fh->ph_entry_size;
	const uint16_t entry_num  = fh->ph_entry_num;

	elf64::program_header* toparse = reinterpret_cast<decltype(toparse)>((uint8_t*)(elf_start) + fh->ph_offset);
	for(uint16_t i = 0; i < entry_num; ++i) {
		if(toparse->type != elf64::program_header::LOAD) {
			if(toparse->type == elf64::program_header::DYNAMIC) {
				return {lk_result::CANNOT_RELOC, nullptr, 0};
			} continue;
		}

		terminal_writestring("Found loadable section ");
		terminal_printhex(toparse->file_off);
		terminal_writestring(" ");
		terminal_printhex(toparse->vaddr);
		terminal_writestring(" ");
		terminal_printhex(toparse->memsz);
		terminal_writestring("\n");

		if(toparse->vaddr < (uint64_t)safezone_start ||
		   toparse->vaddr + toparse->memsz >= (uint64_t)(0x100000000ull)) {
			return {lk_result::SAFEZONE_VIOLATION, nullptr, i};
		}

		uint8_t* memptr = (uint8_t *)(toparse->vaddr);
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
	return {lk_result::OK, (void*)(fh->entrypoint), 0};
}
