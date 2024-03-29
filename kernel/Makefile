CXXFLAGS += -m64 -mno-red-zone -mcmodel=large -DKERNEL
ASFLAGS += -64

KOBJECTS = kernel.o kernel_start.o interrupts.o ../misc/display64.o ../x86/asm_stubs.o \
mem/utils.o mem/phmem_manager.o mem/default_pages.o

kernelfull.o: $(KOBJECTS)
	$(CXX) -r -o kernelfull.o $(KOBJECTS)
kernel_start.o: kernel_start.s
	$(CXX) $(CXXFLAGS) -c kernel_start.s -o kernel_start.o
interrupts.o: interrupts.hh interrupts.cc

../misc/display64.o: ../misc/display.h ../misc/display.cpp
	$(CXX) $(CXXFLAGS) -c ../misc/display.cpp -o ../misc/display64.o
../x86/asm_stubs.o: ../x86/asm_stubs.hh ../x86/asm_stubs.cc
	$(CXX) $(CXXFLAGS) -c ../x86/asm_stubs.cc -o ../x86/asm_stubs.o

kernel.o: kernel.cpp mem/utils.hh mem/vmem_allocator.hh mem/phmem_manager.hh \
mem/phmem_allocator.hh  ../x86/asm_stubs.hh ../x86/memory.hh

mem/utils.o: FORCE
	cd mem && $(MAKE) utils.o
mem/phmem_manager.o: FORCE
	cd mem && $(MAKE) phmem_manager.o

mem/default_pages.o: FORCE
	cd mem && $(MAKE) default_pages.o
FORCE:

.PHONY: clean
clean:
	$(RM) *.o *.bin ../misc/display64.o
	cd mem && $(MAKE) clean
