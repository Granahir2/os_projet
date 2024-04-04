#include "cstring.hh"
#include <cstdint>

extern "C" {

char* strcpy(char* __restrict dest, const char* __restrict src) {
	auto retval = dest;
	for(;(*dest = *src) != '\0'; dest++, src++);
	return retval;
}

char* strncpy(char* __restrict dest, const char*  __restrict src, size_t count) {
	auto retval = dest;
	for(size_t i = count; i > 0 && ((*dest=*src) != '\0'); i--, dest++, src++);
	return retval;
}

char* strcat(char* __restrict dest, const char* __restrict src) {
	auto retval = dest;
	for(;*dest != '\0'; dest++);
	strcpy(dest, src);
	return retval;
}

char* strncat(char* __restrict dest, const char* __restrict src, size_t n) {
	auto retval = dest;
	for(;*dest != '\0'; dest++);
	strncpy(dest, src, n);
	return retval;
}

size_t strlen(const char* x) {
	size_t retval = 0;
	for(;*x != '\0'; retval++,x++);
	return retval;
}

int strcmp(const char* a, const char* b) {
	char delta;
	for(;((delta = *a - *b) == 0) && !(*a == '\0' && *b == '\0'); a++, b++);

	if(delta > 0) {return 1;}
	else if(delta < 0) {return -1;}
	return 0;
}

int strncmp(const char* a, const char* b, size_t n) {
	char delta = 0;
	for(size_t cnt = n; cnt > 0 && ((delta = *a - *b) == 0) && !(*a == '\0' && *b == '\0'); a++,b++);

	if(delta > 0) {return 1;}
	else if(delta < 0) {return -1;}
	return 0;
}


const char* strchr(const char* str, int ch) {
	for(; *str != (unsigned char)ch && *str != '\0'; str++);
	if(*str == (unsigned char)ch) {return str;}
	else {return nullptr;}
}

const char* strrchr(const char* str, int ch) {
	const char* lastfound = nullptr;
	for(; *str != '\0'; str++) {if(*str == (unsigned char)ch) {lastfound = str;}}
	if((unsigned char)ch == '\0'){return str;}
	else {return lastfound;}
}

size_t strspn(const char* dest, const char* src) {
	bool cache[1 << (sizeof(char) * 8)];
	memset(cache, 0, sizeof(char) * 8 * sizeof(bool));

	for(;*src != '\0'; src++) {cache[(unsigned char)*src] = true;}
	size_t r = 0;
	for(;cache[(unsigned char)*dest] == true && *dest != '\0'; dest++, r++);
	return r;
}


size_t strcspn(const char* dest, const char* src) {
	bool cache[1 << (sizeof(char) * 8)];
	memset(cache, true, sizeof(char) * 8 * sizeof(bool));

	for(;*src != '\0'; src++) {cache[(unsigned char)*src] = false;}
	size_t r = 0;
	for(;cache[(unsigned char)*dest] == true && *dest != '\0'; src++, r++);
	return r;
}

const char* strpbrk(const char* dest, const char* src) {
	auto r = strspn(dest, src);
	if(dest[r] == '\0') return nullptr;
	return dest+r;
}

void* memchr(void* s, int c, size_t n) {
	auto p = (unsigned char*)s;
	auto cmp = (unsigned char)c;
	for(size_t count = n; count > 0 && *p != cmp; count--, p++);
	if(*p == cmp) {return p;} else {return NULL;}
}

int memcmp(const void* a, const void* b, size_t n) {
	auto pa = (const char*)a;
	auto pb = (const char*)b;
	char delta;
	size_t count = n;
	for(; count > 0 && (delta = *pa - *pb) == 0; count--);
	
	if(count == 0) {return 0;}
	else if(delta > 0){return 1;} else {return -1;}
}

void memset(void* a, int ch, size_t n) {
	auto c = (unsigned char)ch;
	auto p = (unsigned char*)a;
	for(size_t cnt = n; cnt > 0; cnt--, p++) {*p = c;}
}

void memcpy(void* __restrict dest, const void* __restrict src, size_t n) {
	auto cdest = (unsigned char*)dest;
	auto csrc  = (const unsigned char*)src;

	for(size_t cnt = n; cnt > 0; cnt--, cdest++,csrc++) {*cdest = *csrc;}
}

void memmove(void* dest, const void* src, size_t n) {
	auto cdest = (unsigned char*)dest;
	auto csrc  = (const unsigned char*)src;

	if((intptr_t)(dest) < (intptr_t)(src)) {
		for(size_t cnt = n; cnt > 0; cnt--, cdest++, csrc++) {*cdest = *csrc;}
	} else {
		for(size_t cnt = n; cnt > 0; cnt--) {cdest[cnt-1] = csrc[cnt-1];}
	}
}
}
