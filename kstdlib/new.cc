#include "new.hh"
#include "kernel/kernel.hh"
#include "cstdio.hh"
struct __attribute__((packed)) _smallpage {
	uint64_t slot[502];

	_smallpage* next = nullptr; 
	_smallpage* prev = nullptr;
	uint64_t bitmap[8];
}; static_assert(sizeof(_smallpage) == 4096);

struct {
	_smallpage* entry[503] = {nullptr}; // de 0 à 502 blocs consécutifs libres (les blocs où tout est libre sont discard *SI* la derniere dealloc est une dealloc de masse)
} _head;

unsigned int find_aperture(unsigned slotlen, const _smallpage* pg) {
	unsigned offset = 0;
	for(; offset < 502;) {
		unsigned j = 0;
		for(; j < slotlen && (j + offset < 502)
			         && (((pg->bitmap[(j+offset)/64] >> ((j+offset)%64)) & 1ull) == 0);
				 ++j);
		if(j == slotlen) break;
		offset += j + 1;
	}

	if(offset + slotlen <= 502) {return offset;}
	else{return 503;}
}

unsigned int find_max_apertsize(const _smallpage* pg) {
	unsigned maxsize = 0;
	for(unsigned offset = 0; offset < 502;) {
		unsigned j = 0;
		for(; j + offset < 502 && ((pg->bitmap[(j+offset)/64] >> ((j+offset)%64)) & 1) == 0; ++j);
		if(j > maxsize) {maxsize = j;}
		offset += j + 1;
	}
	return maxsize;
}

void* operator new(size_t len) {
	// Doing new on len == 0 is UB

	// Try to fit allocation in already allocated page
	
	unsigned slotlen = (len + 7) / 8;
	for(unsigned i = slotlen; i < 503; ++i) {
		if(_head.entry[i] != nullptr) {
			_smallpage* pg = _head.entry[i];

			// Naïve search, can surely do faster
			unsigned offset = find_aperture(slotlen, pg);
			assert(offset != 503, "[new] Aperture promised but not found");
			for(unsigned j = 0; j < slotlen; ++j) {
				assert(!((pg->bitmap[(j+offset)/64] >> ((j+offset)%64))&1ull),
					"[new] overwriting alloc");
				pg->bitmap[(j+offset)/64] |= (1ull << ((j+offset)%64));
			}
			auto new_maxapert = find_max_apertsize(pg);

			if(pg->next != nullptr) pg->next->prev = nullptr; // Unlink pg
			_head.entry[i] = pg->next;

			// Relink pg
			if(_head.entry[new_maxapert] != nullptr) _head.entry[new_maxapert]->prev = pg;
			pg->next = _head.entry[new_maxapert];
			pg->prev = nullptr;
			_head.entry[new_maxapert] = pg;
		
			return &(pg->slot[offset]);
		}
	}

	// Allocate new page(s) instead

	// Number of pages required
	size_t numpage = (len + sizeof(_smallpage*)*2 + sizeof(uint64_t)*8 + 4095)/4096; 
	unsigned char* p = (unsigned char*)kmmap(NULL, numpage*4096);
	_smallpage* newpg = (_smallpage*)(&p[4096 * (numpage - 1)]);

	// Number of slots used in last page
	size_t num_lastpg = (len % 4096 + 7) / 8;
	
	for(unsigned i = 0; i < num_lastpg; ++i) {
		newpg->bitmap[i/64] |= (1ull << (i%64));
	}

	for(unsigned i = num_lastpg; i < 502; ++i) {
		newpg->bitmap[i/64] &= ~(1ull << (i%64));
	}
	
	if(numpage > 1) {
		newpg->bitmap[7] |= (1ull << 63); // Mark page as member of a big alloc; don't deallocate automatically if it falls to 0
	}

	if(_head.entry[502-num_lastpg] != nullptr) _head.entry[502 - num_lastpg]->prev = newpg;
	newpg->next = _head.entry[502 - num_lastpg];
	newpg->prev = nullptr;
	_head.entry[502 - num_lastpg] = newpg;

	return p; 
}


void* operator new[](size_t len) {
	size_t* p = (size_t*) operator new(len + sizeof(size_t)); // doesn't clobber alignment
	*p = len;
	return &p[1];
}

void operator delete(void* ptr, size_t len) noexcept {
	if(len == 0 || ptr == nullptr) return;

	//printf("[delete] %p %u\n", ptr, (unsigned)len);

	_smallpage* page;
	unsigned prev_conseq;

	if(((intptr_t)ptr & 0xfff) == 0) { // We *may* be in a big alloc
		size_t numpage = (len + 2*sizeof(_smallpage*) + sizeof(uint64_t)*8  + 4095)/4096;
		//printf("Big dealloc, numpage = %u\n", (unsigned)numpage);

		kmunmap(ptr, (numpage - 1)*4096); // dealloc all head pages

		page = &((_smallpage*)(ptr))[numpage-1];
		prev_conseq = find_max_apertsize(page);

		size_t num_lastpg = (len % 4096 + 7)/8; // number of slots in last page
		for(unsigned i = 0; i < num_lastpg; ++i) {
			if(!((page->bitmap[i/64] >> (i%64)) & 1ull)) {
				printf("double delete ? With i = %u, numpage = %u, num_lastpg = %u\n", i,
					(unsigned)numpage,
					(unsigned)num_lastpg);
				halt();
			}//, "[new] double delete?");

			page->bitmap[i/64] &= ~(1ull << (i%64));
		}
		if(numpage > 1) { // If numpage > 1, we have deallocated the "head" of a big alloc
			page->bitmap[7] &= ~(1ull << 63);
		}
	} else {
		page = (_smallpage*)(((uintptr_t)ptr) & ~0xfffull);
		prev_conseq = find_max_apertsize(page);
		
		unsigned start = ((uintptr_t)ptr % 4096) / 8;
		for(unsigned i = 0; i < (len+7)/8; ++i) {
			assert((page->bitmap[(start+i)/64] >> ((start+i)%64)) & 1ull, "[new] double delete ?");
			page->bitmap[(start+i)/64] &= ~(1ull << ((start+i)%64));
		}
	}


	unsigned curr_conseq = find_max_apertsize(page);

	if(page->prev == nullptr) { // Unlinking page
		if(page->next != nullptr) page->next->prev = nullptr;
		_head.entry[prev_conseq] = page->next;
	} else {
		if(page->next != nullptr) page->next->prev = page->prev;
		page->prev->next = page->next;
	}
	
	bool delete_page = true;
	for(unsigned i = 0; i < 8; ++i) {
		if(page->bitmap[i] != 0) {delete_page = false; break;}
	}
	if(delete_page) {kmunmap(page, 4096); return;}

	// Re-link page
	page->prev = nullptr;
	if(_head.entry[curr_conseq] != nullptr) _head.entry[curr_conseq]->prev = page;
	page->next = _head.entry[curr_conseq];
	_head.entry[curr_conseq] = page;
}

void operator delete[](void* ptr) noexcept{
	if(ptr == nullptr) return;
	auto s = ((size_t*)ptr)[-1];
	operator delete(&(((size_t*)ptr)[-1]), s + sizeof(size_t));
}
