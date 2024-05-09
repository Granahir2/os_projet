#include "hl_command.hh"
#include "kernel/kernel.hh"
#include "kstdlib/cstring.hh"
#include "kernel/posix_except.hh"
#include "kernel/mem/phmem_allocator.hh"

namespace ahci {
eff_command::eff_command(bool write) {
	auto x = get_phpage(); 
	if(x == mem::phalloc_null) {
		throw enomem(); 
	}
	comtab = (decltype(comtab))(x);		
	direction = write;
};

eff_command::~eff_command() {
	release_phpage((x64::phaddr)(comtab));
};

void eff_command::reset(bool ndir) {
	curr_prdt_offset = -1;
	head = nullptr;
	direction = ndir;
	totlen = 0;
}

void eff_command::setup_linear_prdt(x64::phaddr start, size_t len) {
	curr_prdt_offset++;
	if(len % 2 != 0 || start % 2 != 0) {throw einval("Transfers must be word-sized");}

	size_t cnt = 0;
	while(curr_prdt_offset < 248 && cnt < len) {
		size_t delta;
		/*if(direction) { // PRD can always hold their max size, it's only the Data FIS which are capped.
			delta = (len - cnt) < 8196 ? (len - cnt) : 8196;
		} else {*/
			delta = (len - cnt) < 4096*1024 ? (len - cnt) : (4096 * 1024);
		//}
		comtab->prdt[curr_prdt_offset].loc  = start + cnt;
	//	printf("PRD base address %p\n", start+cnt);
		comtab->prdt[curr_prdt_offset].rsv1 = 0;
		comtab->prdt[curr_prdt_offset].dbc  = delta - 1;
		comtab->prdt[curr_prdt_offset].ie   = 0;

		cnt += delta;
		curr_prdt_offset++;
	}

	if(curr_prdt_offset == 248) {
		throw einval(__LINE__ + " Too large transfer");
	}
	totlen += len;
}

void eff_command::setup_header_fis(volatile command_header* there, const void* fis, size_t fis_s) {
	head = there;
	if(!head) {throw einval();}
	memset(head, 0, sizeof(command_header));
	memcpy(&comtab->cfis[0], fis, fis_s);
	
	head->cfl = fis_s / sizeof(uint32_t);
	head->command_table = (x64::phaddr)(&comtab->cfis[0]);
	head->prdt_entrylen   = curr_prdt_offset + 1;
	head->xferred_bytecnt = totlen;
	head->w = direction;
}

}
