export CXXFLAGS= -ggdb -fno-pie -fpic -ffreestanding\
-std=c++23 -mno-mmx -mno-sse -mgeneral-regs-only -fno-stack-protector -I$(CURDIR) -Wall -Wextra\
-nodefaultlibs -fconcepts-diagnostics-depth=10
export CXX=x86_64-elf-g++
export LD=x86_64-elf-ld
export ASFLAGS=-g

CRTI=stdinit/crti.o
CRTN=stdinit/crtn.o
CRT0=stdinit/crt0.o

CRTBEGIN=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)

os.iso: boot/bootstrap.bin klinker.ld kernel/kernelfull.o kstdlib/stdlib.o drivers/drivers.o grub.cfg $(CRT0) $(CRTI) $(CRTN)
	$(CXX) -nostdlib $(CXXFLAGS) -mcmodel=large -static -o kernel.bin -T klinker.ld \
		$(CRT0) $(CRTI) $(CRTBEGIN) \
		kernel/kernelfull.o kstdlib/stdlib.o drivers/drivers.o -lstdc++ -lgcc \
		$(CRTEND) $(CRTN)
	mkdir -p isodir/boot/grub
	cp grub.cfg isodir/boot/grub
	cp boot/bootstrap.bin isodir/boot
	cp kernel.bin isodir/boot
	grub-mkrescue -o os.iso isodir

stdinit/crt*.o: FORCE
	cd stdinit && $(MAKE) $(notdir $@)

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
	qemu-system-x86_64 -cdrom os.iso -s -S  -monitor stdio -no-shutdown -no-reboot -d int
clean:
	$(RM) *.o *.bin *.iso
	$(RM) -r isodir
	cd kernel && $(MAKE) clean
	cd boot && $(MAKE) clean
	cd kstdlib && $(MAKE) clean
	cd drivers && $(MAKE) clean
