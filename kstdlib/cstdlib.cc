#include "cstdlib.hh"
#include "cstdio.hh"
#include <algorithm>

// No overflow control
long int strtol(const char* nptr, const char** endptr, int base) {
	const char* nptr_cpy = nptr;
	bool minusflag = nptr[0] == '-';
	if(minusflag) nptr++;

	bool validpref = false;
	long int result = 0;

	int charValue[256];
	for(int c = 0; c < 256; ++c) {
		if(c - '0' >= 0 && c - '0' <= std::min(base, 9)) {
			charValue[c] = c - '0';
		} else if(c - 'a' >= 0 && c - 'a' <= std::min(base - 10, 26)) {
			charValue[c] = c - 'a';	
		} else if(c - 'A' >= 0 && c - 'A' <= std::min(base - 10, 26)) {
			charValue[c] = c - 'A';
		} else {
			charValue[c] = -1;
		}
	}

	while(charValue[(unsigned char)(*nptr)] != -1) {
		validpref = true;
		result *= base;
		result += charValue[(unsigned char)(*nptr)];
	}
	if(endptr) {
		if(!validpref) {*endptr = nptr_cpy;} else {*endptr = nptr - 1;}
	}
	return result;
}

void* malloc(size_t size) {
	auto* p = new char[size];
	//printf("malloc(%u) = %p\n", (unsigned int)(size), p);
	return p;
}

void free(void* ptr) {
	//printf("free(%p)\n", ptr);
	delete[] ((char*)ptr);
}
