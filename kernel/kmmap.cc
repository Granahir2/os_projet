#include "mem/phmem_allocator.hh"
#include "mem/phmem_manager.hh"
#include "misc/memory_map.hh"
#include "kernel.hh"
#include <new>

mem::ph_tree_allocator<10>* palloc;
mem::VirtualMemoryAllocator<18>* pkvmem_alloc;
mem::phmem_manager* pphmem_man;

unsigned long long min(unsigned long long a, unsigned long long b) {
	if( a < b) {return a;} else {return b;}
}

extern "C" void* kmmap(void* hint, size_t sz) { // The offsets are relative to kernel_zero so that's okay
	if(sz == 0) return NULL;
	auto r = pkvmem_alloc->mmap((x64::linaddr)(hint) + 2*1024*1024*1024ull, sz);
	if(r == mem::alloc_null) {throw std::bad_alloc();}
	pphmem_man->back_vmem(r - 2*1024*1024*1024ull, sz, 0);
	return (void*)(r - 2*1024*1024*1024ull);
}

extern "C" void kmunmap(void* ptr, size_t sz) {
	if(sz == 0) return;
	pkvmem_alloc->munmap((x64::linaddr)ptr + 2*1024*1024*1024ull, sz);
	pphmem_man->unback_vmem((x64::linaddr)ptr, sz, 0);
}

void setup_heap(memory_map_entry* mmap, uint64_t kernel_zero) {

	static mem::ph_tree_allocator<10> alloc;
	static mem::VirtualMemoryAllocator<18> kvmem_alloc;
	static mem::phmem_manager phmem_man(0);

	palloc = &alloc;
	pkvmem_alloc = &kvmem_alloc;
	pphmem_man = &phmem_man;

	kvmem_alloc = mem::VirtualMemoryAllocator<18>();
	kvmem_alloc.mark_used((x64::linaddr)(&kernel_begin) + 2*1024*1024*1024ull,
			(x64::linaddr)(&kernel_end) - (x64::linaddr)(&kernel_begin));

	/*
	printf("Kernel begin:end %p:%p",  (x64::linaddr)(&kernel_begin) + 2*1024*1024*1024ull,
			(x64::linaddr)(&kernel_end) + 2*1024*1024*1024ull);
	*/

	// Mark the physical memory of the kernel as used
	alloc = decltype(alloc)();
	alloc.mark_used((x64::linaddr)(&kernel_begin) + 2*1024*1024*1024ull + kernel_zero,
			(x64::linaddr)(&kernel_end) + 2*1024*1024*1024ull + kernel_zero);

	//printf("Memory map at %p\n", mmap);
	int i = 0;
	for(; mmap[i].length != 0; ++i) {
	/*	printf("%p:%p ", mmap[i].base, mmap[i].base + mmap[i].length - 1, mmap[i].type);
		switch(mmap[i].type) {
			case 1:
				puts("available\n"); break;
			case 3:
				puts("acpi\n"); break;
			case 4:
				puts("hib. safe\n"); break;
			case 5:
				puts("bad ram\n"); break;
			default:
				puts("reserved\n");
		}
	*/	
		if(mmap[i].type != 1 && mmap[i].base < 0x100000000ull) {
			auto end = (mmap[i].base + mmap[i].length - 1);
			if(end >= 0x100000000ull) {end = 0xffffffff;}
			alloc.mark_used(mmap[i].base, end);
		}

		if(i > 0 && mmap[i - 1].base + mmap[i - 1].length < mmap[i].base
			&& mmap[i - 1].base + mmap[i - 1].length < 0x100000000ull) {
			alloc.mark_used(mmap[i - 1].base + mmap[i - 1].length, min(mmap[i].base - 1, (uint64_t)0xffffffffull));
		}
	}
	if(mmap[i-1].base + mmap[i-1].length < 0x100000000ull) {
		alloc.mark_used(mmap[i-1].base + mmap[i-1].length, 0xffffffffull);
	}
}


x64::phaddr get_phpage() {auto x = palloc->get_page(); return x;}
void release_phpage(x64::phaddr p) {palloc->release_page(p);}
