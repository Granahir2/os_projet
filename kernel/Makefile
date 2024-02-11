CXX_FLAGS=-O0 -ggdb -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-pie -fno-pic\
-std=c++20 -mno-mmx -mno-sse -fno-stack-protector -m64 -I.. -Wall -Wextra

kernel.bin: kernel.o kernel_start.o klinker.ld
	ld -T klinker.ld -o kernel.bin kernel.o kernel_start.o
kernel_start.o: kernel_start.s
	as kernel_start.s -64 -o kernel_start.o
kernel.o: kernel.cpp
	g++ $(CXX_FLAGS) -c kernel.cpp -o kernel.o

.PHONY: clean
clean:
	rm *.o *.bin
