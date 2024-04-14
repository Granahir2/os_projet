CXXFLAGS += -m32
ASFLAGS += -32
bootstrap.bin: x64bootstrap.o load_kernel.o boot.o display32.o linker.ld
	$(LD) -g -T linker.ld -melf_i386 -o bootstrap.bin x64bootstrap.o load_kernel.o boot.o display32.o

boot.o: boot.s
x64bootstrap.o: x64bootstrap.cpp multiboot.hh load_kernel.hh ../x86/descriptors.hh
load_kernel.o: load_kernel.cpp load_kernel.hh
display32.o: ../misc/display.h ../misc/display.cpp
	$(CXX) $(CXXFLAGS) -c -o display32.o ../misc/display.cpp


.PHONY: clean
clean:
	$(RM) *.o *.bin


