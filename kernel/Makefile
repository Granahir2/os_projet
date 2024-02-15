CXX_FLAGS=-O0 -ggdb -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-pie -fno-pic\
-std=c++2a -mno-mmx -mno-sse -fno-stack-protector -m64 -I.. -Wall -Wextra

kernel.bin: kernel.o kernel_start.o klinker.ld ../misc/display64.o
	ld -T klinker.ld -o kernel.bin kernel.o kernel_start.o ../misc/display64.o
kernel_start.o: kernel_start.s
	as kernel_start.s -64 -o kernel_start.o
kernel.o: kernel.cpp
	g++ $(CXX_FLAGS) -c kernel.cpp -o kernel.o
../misc/display64.o : ../misc/display.h ../misc/display.cpp
	g++ $(CXX_FLAGS) -c ../misc/display.cpp -o ../misc/display64.o

.PHONY: clean
clean:
	rm -f *.o *.bin ../misc/display64.o
