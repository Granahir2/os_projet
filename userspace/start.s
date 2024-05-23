.section .bss
.align 16
.skip 2048*1024
stack_top:


.section .text
.global _start
.type _start, @function
_start:
	movabs $stack_top, %rsp
	mov %rdi, stdout
	mov %rsi, stdin
	call main
loop:	int $32
	jmp loop
