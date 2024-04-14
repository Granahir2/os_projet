CXXFLAGS += -m64 -mno-red-zone -mcmodel=large -DKERNEL -I$(CURDIR)
ASFLAGS += -64

KOBJECTS = kernel.o kmmap.o interrupts.o ../misc/display64.o ../x86/asm_stubs.o \
mem/utils.o mem/phmem_manager.o mem/default_pages.o fs/rootfs.o fs/devfs.o

kernelfull.o: $(KOBJECTS)
	$(CXX) -r -o kernelfull.o $(KOBJECTS)
interrupts.o: interrupts.hh interrupts.cc
kmmap.o: kmmap.cc kernel.hh #to complete

../misc/display64.o: ../misc/display.h ../misc/display.cpp
	$(CXX) $(CXXFLAGS) -c ../misc/display.cpp -o ../misc/display64.o
../x86/asm_stubs.o: ../x86/asm_stubs.hh ../x86/asm_stubs.cc
	$(CXX) $(CXXFLAGS) -c ../x86/asm_stubs.cc -o ../x86/asm_stubs.o

kernel.o: kernel.cpp mem/utils.hh mem/vmem_allocator.hh mem/phmem_manager.hh \
mem/phmem_allocator.hh  ../x86/asm_stubs.hh ../x86/memory.hh

fs/rootfs.o: FORCE
	cd fs && $(MAKE) rootfs.o
fs/devfs.o: FORCE
	cd fs && $(MAKE) devfs.o

mem/mem.o: FORCE
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
	cd fs && $(MAKE) clean
