CXXFLAGS += -m64 -mno-red-zone -mcmodel=large -DKERNEL
ASFLAGS += -64

kernel.bin: klinker.ld kernel.o kernel_start.o interrupts.o pci.o ../misc/display64.o ../x86/asm_stubs.o \
mem/utils.o mem/phmem_manager.o
	ld -o kernel.bin -T $^

kernel_start.o: kernel_start.s
	$(CXX) $(CXXFLAGS) -c kernel_start.s -o kernel_start.o
interrupts.o: interrupts.hh interrupts.cc
pci.o: pci.hh pci.cc
../misc/display64.o : ../misc/display.h ../misc/display.cpp
	$(CXX) $(CXXFLAGS) -c ../misc/display.cpp -o ../misc/display64.o
../x86/asm_stubs.o : ../x86/asm_stubs.hh ../x86/asm_stubs.cc

kernel.o: kernel.cpp mem/utils.hh mem/vmem_allocator.hh mem/phmem_manager.hh ../x86/asm_stubs.hh ../x86/memory.hh

.PHONY: mem/utils.o mem/phmem_manager.o
mem/utils.o:
	cd mem && $(MAKE) utils.o
mem/phmem_manager.o:
	cd mem && $(MAKE) phmem_manager.o

.PHONY: clean
clean:
	$(RM) *.o *.bin ../misc/display64.o
	cd mem && $(MAKE) clean
