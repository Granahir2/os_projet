#pragma once
#include <cstdint>

namespace elf {

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

}
