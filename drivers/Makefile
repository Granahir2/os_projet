export CXXFLAGS += -m64 -mno-red-zone -mcmodel=large -fpic -O3
drivers.o: serial/serial.o
	g++ -r $^ -o drivers.o

serial/serial.o: FORCE
	cd serial && $(MAKE)
FORCE:

.PHONY: clean
clean:
	cd serial && $(MAKE) clean
