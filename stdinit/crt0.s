/* From https://osdev.org/Bare_Bones, used with discretion */

.section .bss
.align 16
stack_bottom:
.skip 2048*1024 # 2 MiB
stack_top:

istack_bottom:
.skip 16384
istack_top:

/*
The linker script specifies _start as the entry point to the kernel and the
bootloader will jump to this position once the kernel has been loaded. It
doesn't make sense to return from this function as the bootloader is gone.
*/
.section .text
.global _start
.type _start, @function
_start:
	/* We are in 64 bit mode with flat segmentation and no paging */
	movabs $stack_top, %rsp	
	movabs $istack_top, %rdi
	
	call kernel_early_main
	call _init
	call landing_pad # provides nicer crash handling
	call _fini

/* convenience function, used for clean halting */
.global halt
.type halt,@function
halt:	cli
1:	hlt
	jmp 1b

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.

.size _start, . - _start
*/
