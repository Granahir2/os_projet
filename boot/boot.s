/* From https://osdev.org/Bare_Bones, used with discretion */

/*
The multiboot standard does not define the value of the stack pointer register
(esp) and it is up to the kernel to provide a stack. This allocates room for a
small stack by creating a symbol at the bottom of it, then allocating 16384
bytes for it, and finally creating a symbol at the top. The stack grows
downwards on x86. The stack is in its own section so it can be marked nobits,
which means the kernel file is smaller because it does not contain an
uninitialized stack. The stack on x86 must be 16-byte aligned according to the
System V ABI standard and de-facto extensions. The compiler will assume the
stack is properly aligned and failure to align the stack will result in
undefined behavior.
*/
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.global kernel_zero
kernel_zero:
.skip 8 # it's a uint64_t

/*
The linker script specifies _start as the entry point to the kernel and the
bootloader will jump to this position once the kernel has been loaded. It
doesn't make sense to return from this function as the bootloader is gone.
*/
.section .text
.global _start
.type _start, @function
_start:
	/*
	The bootloader has loaded us into 32-bit protected mode on a x86
	machine. Interrupts are disabled. Paging is disabled. The processor
	state is as defined in the multiboot standard. The kernel has full
	control of the CPU. The kernel can only make use of hardware features
	and any code it provides as part of itself. There's no printf
	function, unless the kernel provides its own <stdio.h> header and a
	printf implementation. There are no security restrictions, no
	safeguards, no debugging mechanisms, only what the kernel provides
	itself. It has absolute and complete power over the
	machine.
	*/

	/*
	To set up a stack, we set the esp register to point to the top of the
	stack (as it grows downwards on x86 systems). This is necessarily done
	in assembly as languages such as C cannot function without a stack.
	*/
	mov $stack_top, %esp
	pushl %ebx # info struct ptr
	pushl %eax # multiboot magic value

	call ready_gdt_desc 

	lgdt early_gdt_desc

	ljmp $0x08, $.dummy_target
.dummy_target:
	mov $0x10,%cx
	mov %cx,%ds
	mov %cx,%es
	mov %cx,%fs
	mov %cx,%gs
	mov %cx,%ss

	/*
	Enter the high-level kernel. The ABI requires the stack is 16-byte
	aligned at the time of the call instruction (which afterwards pushes
	the return pointer of size 4 bytes). The stack was originally 16-byte
	aligned above and we've pushed a multiple of 16 bytes to the
	stack since (pushed 0 bytes so far), so the alignment has thus been
	preserved and the call is well defined.
	*/
	call bootstrap

	pushl $0xffffffff
	pushl %eax

	mov %cr4, %eax
	or  $0x20, %eax
	mov %eax, %cr4
	
	mov $level4_page_table, %eax
	mov %eax, %cr3

	mov $0, %edx
	mov $0x100, %eax
	mov $0xC0000080, %ecx
	wrmsr # Enable long mode !

	mov %cr0, %ebx
	or $0x80000001, %ebx
	mov %ebx, %cr0

	lgdt early_gdt_desc # Reload GDT

	mov $0x10,%cx
	mov %cx,%ds
	mov %cx,%es
	mov %cx,%fs
	mov %cx,%gs
	mov %cx,%ss

	# We are in compatibility mode. We now jump to long mode
	mov $memory_map_buffer, %esi

	mov $kernel_zero, %eax
	mov 0(%eax), %edx # Placeholder for kernel_zero

	ljmp $0x08, $.longmode_gate
.longmode_gate:
	ret
	/*
	If the system has nothing more to do, put the computer into an
	infinite loop. To do that:
	1) Disable interrupts with cli (clear interrupt enable in eflags).
	   They are already disabled by the bootloader, so this is not needed.
	   Mind that you might later enable interrupts and return from
	   kernel_main (which is sort of nonsensical to do).
	2) Wait for the next interrupt to arrive with hlt (halt instruction).
	   Since they are disabled, this will lock up the computer.
	3) Jump to the hlt instruction if it ever wakes up due to a
	   non-maskable interrupt occurring or due to system management mode.
	*/
.global halt
.type halt,@function
halt:	cli
1:	hlt
	jmp 1b

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
