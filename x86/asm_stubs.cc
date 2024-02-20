#include "asm_stubs.hh"


void kprintf(const char* str, ...);
namespace x64 {



void lgdt(uint64_t) {
	asm(R"foo(
	lgdt 0(%rdi)
	)foo"); 
}


void lidt(uint64_t) {
	asm(R"foo(
	lidt 0(%rdi)
	)foo"); 
}

void reload_descriptors() {
	asm(R"foo(
	mov $0x10,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
	mov %ax,%ss
	push $0x08
	mov $1f, %rax
	push %rax
	lretq
	1:
	)foo");
}

void reload_tss(uint16_t) {
	asm(R"foo(
	ltr %di
	)foo");
}

void sei() {
	asm("sti");
}
void inb(uint16_t) {
	asm(R"foo(
	push %rdx
	mov %rdi, %rdx
	xor %rax, %rax, %rax
	in %dx, %al
	pop %rdx
	)foo");
}
void outb(uint16_t, uint8_t) {
	asm(R"foo(
	push %rdx
	mov %rdi, %rdx
	mov %rsi, %rax
	out %al, %dx
	pop %rdx
	)foo");
}

uint64_t __attribute__((naked)) rdmsr(uint32_t) {
	asm(R"foo(
	push %rcx
	push %rdx
	mov %edi, %ecx
	movabs $0x0, %rax
	rdmsr
	shl $0x32, %rdx
	add %rdx, %rax
	pop %rdx
	pop %rcx
	ret
	)foo");
}
}
