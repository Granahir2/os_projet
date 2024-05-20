.globl saveregs_hook
.type saveregs_hook, @function
.align 8

.globl registers
.globl print_regs

saveregs_hook: # Made to be called immediately from the scheduler gate ISR
	mov %rax, (registers)
	mov %rbx, (registers+8)
	mov %rcx, (registers+16)
	mov %rdx, (registers+24)
	mov %rsi, (registers+32)
	mov %rdi, (registers+40)
	mov %rbp, (registers+48)
	
	mov 24(%rsp), %rax
	mov %rax, (registers+56) /*rsp*/

	mov 16(%rsp), %rax
	mov %rax, (registers+640) /*rflags*/

	mov 0(%rsp), %rax
	mov %rax, (registers+648) /*rip*/

	mov 8(%rsp), %rax
	mov %rax, (registers+656) /*cs*/

	mov 32(%rsp), %rax
	mov %rax, (registers+664) /*ss*/ 

	mov %r8, (registers+64)
	mov %r9, (registers+72)
	mov %r10, (registers+80)
	mov %r11, (registers+88)
	mov %r12, (registers+96)
	mov %r13, (registers+104)
	mov %r14, (registers+112)
	mov %r15, (registers+120)
	
	fxsave registers+128
	mov %cr3, %rax
	mov %rax, registers+672

	mov %rsp, %rbp
	cld
	call scheduler_main


.globl loadregs_hook
.type loadregs_hook, @function
.align 8

loadregs_hook:
	cli
	fxrstor registers+128
	mov %r15, (registers+120)
	mov %r14, (registers+112)
	mov %r13, (registers+104)
	mov %r12, (registers+96)
	mov %r11, (registers+88)
	mov %r10, (registers+80)
	mov %r9,  (registers+72)
	mov %r8,  (registers+64)

	mov (registers+664), %rax /*ss*/
	mov %rax, 32(%rsp)
	mov (registers+656), %rax /*cs*/
	mov %rax, 8(%rsp)
	mov (registers+648), %rax /*rip*/
	mov %rax, 0(%rsp)
	mov (registers+640), %rax /*rflags*/
	mov %rax, 16(%rsp)
	mov (registers+ 56), %rax /*rsp*/
	mov %rax, 24(%rsp)

	mov (registers+672), %rax
	mov %rax, %cr3

	mov (registers), %rax
	mov (registers+8), %rbx
	mov (registers+16), %rcx
	mov (registers+24), %rdx
	mov (registers+32), %rsi
	mov (registers+40), %rdi
	mov (registers+48), %rbp	
	sti
	iretq

.globl syscall_hook
.type syscall_hook, @function
.align 8
syscall_hook:
	call syscall_main
	iretq
