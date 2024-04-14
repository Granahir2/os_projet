export CXXFLAGS += -m64 -mno-red-zone -mcmodel=large -fpic -O3
drivers.o: serial/serial.o vga_text/vga_text.o
	g++ -r $^ -o drivers.o

vga_text/vga_text.o: FORCE
	cd vga_text && $(MAKE)
serial/serial.o: FORCE
	cd serial && $(MAKE)
FORCE:

.PHONY: clean
clean:
	cd serial && $(MAKE) clean
	cd vga_text && $(MAKE) clean
