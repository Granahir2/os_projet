export CXXFLAGS= -ggdb -fno-rtti -fno-pie -fpic -fno-exceptions -ffreestanding\
-std=c++2a -mno-mmx -mno-sse -mgeneral-regs-only -fno-stack-protector -I$(CURDIR) -Wall -Wextra\
-nostdlib

export ASFLAGS=-g
os.iso: boot/bootstrap.bin klinker.ld kernel/kernelfull.o kstdlib/stdlib.o drivers/drivers.o grub.cfg
	$(CXX) -nostartfiles $(CXXFLAGS) -mcmodel=large -static -o kernel.bin -lgcc -lstdc++ -T klinker.ld kernel/kernelfull.o kstdlib/stdlib.o drivers/drivers.o
	mkdir -p isodir/boot/grub
	cp grub.cfg isodir/boot/grub
	cp boot/bootstrap.bin isodir/boot
	cp kernel.bin isodir/boot
	grub-mkrescue -o os.iso isodir

misc: FORCE
	cd misc && $(MAKE)
boot/bootstrap.bin: FORCE
	cd boot && $(MAKE)
kernel/kernelfull.o: FORCE
	cd kernel && $(MAKE)
drivers/drivers.o: FORCE
	cd drivers && $(MAKE)
kstdlib/stdlib.o: FORCE
	cd kstdlib && $(MAKE)

FORCE:

.PHONY: clean test debug
test: os.iso
	qemu-system-x86_64 -cdrom os.iso -m 4G -serial stdio | tee log.txt
debug: os.iso
	qemu-system-x86_64 -cdrom os.iso -s -S -monitor stdio -no-shutdown -no-reboot -d int
clean:
	$(RM) *.o *.bin *.iso
	$(RM) -r isodir
	cd kernel && $(MAKE) clean
	cd boot && $(MAKE) clean
	cd kstdlib && $(MAKE) clean
	cd drivers && $(MAKE) clean
