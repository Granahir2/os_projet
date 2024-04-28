export CXXFLAGS += -m64 -mno-red-zone -mcmodel=large -fpic -O3
drivers.o: serial/serial.o vga_text/vga_text.o gpt/gpt.o fat/fat.o
	$(CXX) $(CXXFLAGS) -r $^ -o drivers.o

vga_text/vga_text.o: FORCE
	cd vga_text && $(MAKE)
serial/serial.o: FORCE
	cd serial && $(MAKE)
gpt/gpt.o: FORCE
	cd gpt && $(MAKE)

fat/fat.o: FORCE
	cd fat && $(MAKE)
FORCE:

.PHONY: clean
clean:
	cd serial && $(MAKE) clean
	cd vga_text && $(MAKE) clean
	cd gpt && $(MAKE) clean
	cd fat && $(MAKE) clean
