#include "user_libc.hh"

FILE* stdout;
FILE* stdin;

long __attribute__((naked))
syscall(int callnum, void* ptr = nullptr, long arg1 = 0, long arg2 = 0, long arg3 = 0) {
	asm(R"foo(
		int $33
		ret
	)foo");
} 

/* rdi, rsi, rdx, rcx, r8, r9 */
size_t fread(void* buffer, size_t sz, size_t nmemb, FILE* f) {
// Technically not compliant because we could stop midway through an element...
	return syscall(0, f, (long)buffer, sz*nmemb);
}

size_t fwrite(const void* buffer, size_t sz, size_t nmemb, FILE* f) {
	return syscall(1, f, (long)buffer, sz*nmemb);
}

DIR* get_root() {
	return (DIR*)syscall(4);
}

drit_status traverse(const char* name, DIR* d) {
	return (drit_status)syscall(3, d, (long)name);	
}

void canonical_path(DIR* d, char* buffer, size_t n) {
	syscall(5, d, (long)buffer, n);
}
