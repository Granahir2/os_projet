#include "cstdio.hh"
#include "cstring.hh"
#include "kernel/kernel.hh"
#include "stdarg.h"

extern "C" {
FILE* stdout = nullptr;

size_t fwrite(const void* buffer, size_t size, size_t cnt, FILE* f) {
	auto p = (unsigned char*)buffer;
	for(size_t obj = 0; obj < cnt; ++obj) {
		for(size_t i = 0; i < size; i += f->write((const void*)(&p[i]), size - i));
		p += size;
	}
	return cnt; // naïf
}

int puts(const char* src) {
	size_t n = strlen(src);
	fwrite(src, n, 1, stdout);
	return 0; // naïf
}
}
template<typename T>
int print_number(T x, char* buf, int base, int minnum = 1, bool upper = false, bool sign = false) {
	if(base == 0) return 0;

	const char lutLower[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	const char lutUpper[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	const char* lut = upper ? lutUpper : lutLower;

	if(x == 0) {buf[0] = '0'; return 1;}
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
int vaprintf(const char* src, va_list arglist) {
	auto s = strchr(src, '%');
	if(s == NULL) {
		puts(src); return strlen(src);
	}

	auto written = fwrite(src, s - src, 1, stdout); 
	
	int off = 2; // number of bytes to jump forward
	switch(s[1]) {
		case '%':
			puts("%");break;
		case 'd':
			{int x = va_arg(arglist, int);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 10, 1, false, true);
			 puts(buffer);} break;
		case 'x':
			{int x = va_arg(arglist, int);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 16);
			 puts(buffer);} break;
		case 'X':
			{int x = va_arg(arglist, int);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 16,true);
			 puts(buffer);} break;
		case 'u':
			{unsigned int x = va_arg(arglist, unsigned int);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 10);
			 puts(buffer);} break;
		case 'p':
			{uintptr_t x = va_arg(arglist, uintptr_t);
			 char buffer[sizeof(x)*8 + 2];
			 memset(buffer, 0, sizeof(x)*8 + 2);
			 written += print_number(x, buffer, 16, sizeof(x)*2);
			 puts(buffer);} break;
		case 'c':
			{int ch = va_arg(arglist, int);
			 unsigned char buffer[2] = {(unsigned char)ch, 0};
			 written += 1; 
			 puts((char*)buffer);} break;
		case 's':
			{char* x = va_arg(arglist, char*);
			 puts(x);
			 written += strlen(x);} break;
	}
	return written + vaprintf(s + off, arglist);
}

int printf(const char* src,...) {
	va_list arglist;
	va_start(arglist, src);
	auto r = vaprintf(src, arglist);
	va_end(arglist);
	return r;
}
/*
int sscanf(const char* src, const char* format, ...){
	va_list arglist;
	va_start(arglist, format);
	auto r = vasscanf(src, arglist);
	va_end(arglist);
	return r;
}
*/
}
