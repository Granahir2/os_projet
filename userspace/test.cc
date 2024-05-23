#include "user_libc.hh"
#include "stdarg.h"


extern "C" {

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

void memset(volatile void* a, int ch, size_t n) {
	auto c = (unsigned char)ch;
	auto p = (unsigned char*)a;
	for(size_t cnt = n; cnt > 0; cnt--, p++) {*p = c;}
}

void memcpy(volatile void* __restrict dest, const void* __restrict src, size_t n) {
	auto cdest = (unsigned char*)dest;
	auto csrc  = (const unsigned char*)src;

	for(size_t cnt = n; cnt > 0; cnt--, cdest++,csrc++) {*cdest = *csrc;}
}

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
	for(size_t cnt = n; cnt > 0 && ((delta = *a - *b) == 0) && !(*a == '\0' && *b == '\0');
	a++,b++,cnt--);

	if(delta > 0) {return 1;}
	else if(delta < 0) {return -1;}
	return 0;
}


const char* strchr(const char* str, int ch) {
	for(; *str != (unsigned char)ch && *str != '\0'; str++);
	if(*str == (unsigned char)ch) {return str;}
	else {return nullptr;}
}

char* strchr(char* str, int ch) {
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

char* strrchr(char* str, int ch) {
	char* lastfound = nullptr;
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

// very naïve version
const char* strstr(const char* str, const char* substr) {
	const char* base = str;
	size_t n = strlen(substr);
	while(*base != '\0') {
		if(strncmp(base, substr, n) == 0) {return base;}
		base++;
	}
	return nullptr;
}


int fputc(int c, FILE* stream) {
	fwrite(&c, 1, 1, stream);
	return (unsigned char)c;
} 

int putchar(int c) {return fputc(c, stdout);}

int fputs(const char* src, FILE* stream) {
	size_t n = strlen(src);
	fwrite(src, n, 1, stream);
	return 0; // naïf
}

int puts(const char* src) {
	auto x = fputs(src, stdout); putchar('\n'); return x;
}


template<typename T>
int print_number(T x, char* buf, int base, int minnum = 1, bool upper = false, bool sign = false) {
	if(base == 0) return 0;

	const char lutLower[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	const char lutUpper[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	const char* lut = upper ? lutUpper : lutLower;

	if(x == 0 && minnum == 1) {buf[0] = '0'; return 1;}
	if(sign && x < 0)  {buf[0] = '-'; return print_number(-x, &buf[1], base);}

	
	if(2 <= base && base <= 16) {
		int n = 0;
		T maxpow = 1;
		for(; x/maxpow >= (T)(base); maxpow *= base, n++);
		
		T acc = x;
		if(n < minnum - 1) n = minnum - 1;
		for(int i = 0; i <= n; ++i) {
			buf[n - i] = lut[acc % base];
			acc /= base;
		}
		return n;	
	}
	return 0;
}

extern "C" {
int vafprintf(FILE* stream, const char* src, va_list arglist) {
	auto s = strchr(src, '%');
	if(s == NULL) {
		fputs(src, stream); return strlen(src);
	}

	auto written = fwrite(src, s - src, 1, stream); 
	
	int off = 2; // number of bytes to jump forward
	switch(s[1]) {
		case '%':
			puts("%");break;
		case 'd':
			{int x = va_arg(arglist, int);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 10, 1, false, true);
			 fputs(buffer, stream);} break;
		case 'x':
			{unsigned x = va_arg(arglist, unsigned int);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 16, sizeof(x)*2);
			 fputs(buffer, stream);} break;
		case 'X':
			{unsigned x = va_arg(arglist, unsigned int);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 16, sizeof(x)*2, true);
			 fputs(buffer, stream);} break;
		case 'u':
			{unsigned int x = va_arg(arglist, unsigned int);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 10);
			 fputs(buffer, stream);} break;
		case 'l':
			switch(s[2]) { // We have LL = L so we get away not supporting %ll{u,d}
				case 'u':
					{unsigned long x = va_arg(arglist, unsigned long);
		 			char buffer[sizeof(x)*8 + 2];
		 			memset(buffer, 0, sizeof(x)*8 + 2);
					written += print_number(x, buffer, 10);
		 			fputs(buffer, stream); off++;} break;
				case 'd':
					{long x = va_arg(arglist, long);
		 			char buffer[sizeof(x)*8 + 2];
		 			memset(buffer, 0, sizeof(x)*8 + 2);
					written += print_number(x, buffer, 10, 1, false, true);
		 			fputs(buffer, stream); off++;} break;
				case 'x':
					{unsigned long x = va_arg(arglist, unsigned long);
					 char buffer[sizeof(x)*8 + 2];
					 memset(buffer, 0, sizeof(x)*8 + 2);
					 written += print_number(x, buffer, 16, sizeof(x)*2);
					 fputs(buffer, stream); off++;} break;
					} break;
		case 'p':
			{uintptr_t x = va_arg(arglist, uintptr_t);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 16, sizeof(x)*2);
			 fputs(buffer, stream);} break;
		case 'c':
			{int ch = va_arg(arglist, int);
			 unsigned char buffer[2] = {(unsigned char)ch, 0};
			 written += 1; 
			 fputs((char*)buffer, stream);} break;
		case 's':
			{char* x = va_arg(arglist, char*);
			 fputs(x, stream);
			 written += strlen(x);} break;
	}
	return written + vafprintf(stream, s + off, arglist);
}

int fprintf(FILE* stream, const char* src,...) {
	va_list arglist;
	va_start(arglist, src);
	auto r = vafprintf(stream, src, arglist);
	va_end(arglist);
	return r;
}

int printf(const char* src, ...) {
	va_list arglist;
	va_start(arglist, src);
	auto r = vafprintf(stdout, src, arglist);
	va_end(arglist);
	return r;
}

const char* str = "Hello from C userspace!";

int main() {
	puts(str);
	if(stdin == nullptr) {return 0;}
	DIR* d = get_root();
	while(true) {
		char buf[256];
		canonical_path(d, buf, 256);
		printf("%s>", buf);

		size_t totsize = 0;
		char buffer[257];

		while(totsize == 0 || buffer[totsize - 1] >= 32) {
			asm("int $32");
			size_t s = fread(&buffer[totsize], 1, 256-totsize, stdin);
			if(s) {
				if(buffer[totsize + s - 1] < 32) {
					fwrite(&buffer[totsize], 1, s-1, stdout);
				} else {
					fwrite(&buffer[totsize], 1, s, stdout);
				}
			} else continue;
			totsize += s;
		} 
		putchar('\n');

		buffer[totsize - 1] = '\0';
        // Taking backspaces into account
        size_t itr1 = 0, itr2 = 0;
        while (itr1 < totsize)
        {
            if (buffer[itr1] == 127)
            {
                if (itr2 > 0) itr2--;
            }
            else buffer[itr2++] = buffer[itr1];
            itr1++;
        }
        buffer[itr2++] = '\0';
        totsize = itr2;

		traverse(&buffer[0], d);

		buffer[256] = '\0';

		size_t listingsz = list(d, buffer, 256);
		
		if(listingsz == 256) {
			puts("directory too long to hold in buffer\n");
		} else {
			for(int i = 0; i < listingsz; ++i) {
				printf("%s\n", &buffer[i]);
				i += strlen(&buffer[i]);
			}
		}	
	}

	return 0;
}

}
