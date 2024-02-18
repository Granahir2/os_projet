#pragma once
#include <stdint.h>
/*
* Given a pointer to an ELF64 file, tries to load it. Fails if the image
* loads addresses below safezone_start. This prevents the bootstrapper/image file
* from being overwritten half-way through the process.
* The ELF file may not be relocatable and may not occupy memory beyond the lower
* 4 GiB.
* If loading succeeded, the result contains OK and a void* to the kernel entry point.
* If loading failed, the first and last fields of the return value contain a diagnostic.
*/
struct lk_result {
	enum {OK, BAD_MAGIC, BAD_ELF, NOT_EXEC, NOT_X64,
		CANNOT_RELOC, SAFEZONE_VIOLATION} result_code;
	void* entrypoint;
	int diagnostic;
};

lk_result load_kernel(uint32_t safezone_start, void* elf_start);
