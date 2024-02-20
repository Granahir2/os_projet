CXXFLAGS += -m64 -mno-red-zone
ASFLAGS += -64

kernel.bin: klinker.ld kernel.o kernel_start.o interrupts.o ../misc/display64.o ../x86/asm_stubs.o
	ld -o $@ $^

kernel_start.o: kernel_start.s
interrupts.o: interrupts.hh interrupts.cc

../misc/display64.o : ../misc/display.h ../misc/display.cpp
	$(CXX) $(CXXFLAGS) -c ../misc/display.cpp -o ../misc/display64.o
../x86/asm_stubs.o : ../x86/asm_stubs.hh ../x86/asm_stubs.cc

.PHONY: clean
clean:
	$(RM) *.o *.bin ../misc/display64.o
