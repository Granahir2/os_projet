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
	movabs $1f, %rax
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

void enable_sse() {
	asm(R"foo(
	mov %cr0, %rax
	and $0xfffb, %ax
	or  $0x2, %ax
	mov %rax, %cr0
	mov %cr4, %rax
	or  $0x600, %rax
	mov %rax, %cr4
	)foo");
}

void sei() {
	asm("sti");
}
void cli() {
	asm("cli");
}
uint8_t __attribute__((naked)) inb(uint16_t) {
	asm(R"foo(
	push %rdx
	mov %rdi, %rdx
	movabs $0x0, %rax
	in (%dx), %al
	pop %rdx
	ret
	)foo");
	return 0;
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


uint32_t __attribute__((naked)) inl(uint16_t) {
	asm(R"foo(
	push %rdx
	mov %rdi, %rdx
	movabs $0x0, %rax
	in (%dx), %eax
	pop %rdx
	ret
	)foo");
	return 0;
}
void outl(uint16_t, uint32_t) {
	asm(R"foo(
	push %rdx
	mov %rdi, %rdx
	mov %rsi, %rax
	out %eax, %dx
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

x64::phaddr __attribute__((naked)) get_cr3() {
	asm(R"foo(
	mov %cr3, %rax
	ret)foo");
}

void __attribute__((naked)) load_cr3(x64::phaddr) {
	asm(R"foo(
	mov %rdi, %cr3
	ret)foo");
}
}
