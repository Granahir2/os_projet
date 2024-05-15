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
	ret


.globl loadregs_hook
.type loadregs_hook, @function
.align 8

loadregs_hook:
	cli
	sub %rsp, 40
	fxrstor registers+128
	mov %r15, (registers+120)
	mov %r14, (registers+112)
	mov %r13, (registers+104)
	mov %r12, (registers+96)
	mov %r11, (registers+88)
	mov %r10, (registers+80)
	mov %r9,  (registers+72)
	mov %r8,  (registers+64)

	mov (registers+664), %rax
	mov %rax, 32(%rsp)
	mov (registers+656), %rax
	mov %rax, 8(%rsp)
	mov (registers+648), %rax
	mov %rax, 0(%rsp)
	mov (registers+640), %rax
	mov %rax, 16(%rsp)
	mov (registers+ 56), %rax
	mov %rax, 24(%rsp)

	mov %rax, (registers)
	mov %rbx, (registers+8)
	mov %rcx, (registers+16)
	mov %rdx, (registers+24)
	mov %rsi, (registers+32)
	mov %rdi, (registers+40)
	mov %rbp, (registers+48)	
	sti
	iretq
