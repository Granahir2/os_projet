os.iso: bootstrap kernel grub.cfg
	mkdir -p isodir/boot/grub
	cp grub.cfg isodir/boot/grub
	cp boot/bootstrap.bin isodir/boot
	cp kernel/kernel.bin isodir/boot
	grub-mkrescue -o os.iso isodir

bootstrap:
	cd boot && $(MAKE)
kernel:
	cd kernel && $(MAKE)

.PHONY: clean test kernel bootstrap
test: os.iso
	qemu-system-x86_64 -cdrom os.iso
clean:
	rm *.o *.bin *.iso
	rm -rf isodir
	cd kernel && $(MAKE) clean
