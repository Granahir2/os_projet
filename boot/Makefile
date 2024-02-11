CXX_FLAGS=-O0 -ggdb -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-pie -fno-pic\
-std=c++20 -mno-mmx -mno-sse -fno-stack-protector -m32 -I.. -Wall -Wextra

bootstrap.bin: x64bootstrap.o load_kernel.o boot.o linker.ld
	ld -g -T linker.ld -melf_i386 -o bootstrap.bin x64bootstrap.o load_kernel.o boot.o 
boot.o: boot.s
	as -g -32 -o boot.o boot.s
x64bootstrap.o: x64bootstrap.cpp multiboot.hh load_kernel.hh ../x86/descriptors.hh
	g++ $(CXX_FLAGS) -c -o x64bootstrap.o x64bootstrap.cpp
load_kernel.o: load_kernel.cpp load_kernel.hh
	g++ $(CXX_FLAGS) -c -o load_kernel.o load_kernel.cpp

.PHONY: clean
clean:
	rm *.o *.bin


