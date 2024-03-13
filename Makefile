export CXXFLAGS=-O0 -ggdb -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-pie -fno-pic\
-std=c++2a -mno-mmx -mno-sse -mgeneral-regs-only -fno-stack-protector -I$(CURDIR) -Wall -Wextra
export ASFLAGS=-g
os.iso: bootstrap kernel grub.cfg
	mkdir -p isodir/boot/grub
	cp grub.cfg isodir/boot/grub
	cp boot/bootstrap.bin isodir/boot
	cp kernel/kernel.bin isodir/boot
	grub-mkrescue -o os.iso isodir

misc:
	cd misc && $(MAKE)

bootstrap:
	cd boot && $(MAKE)
kernel:
	cd kernel && $(MAKE)

.PHONY: clean test kernel bootstrap misc
test: os.iso
	qemu-system-x86_64 -cdrom os.iso
debug: os.iso
	qemu-system-x86_64 -cdrom os.iso -s -S -monitor stdio

clean:
	$(RM) *.o *.bin *.iso
	$(RM) -r isodir
	cd kernel && $(MAKE) clean
	cd boot && $(MAKE) clean
