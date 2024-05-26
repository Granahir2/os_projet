CXXFLAGS += -nostartfiles -nodefaultlibs -no-pie -fno-pie
test.elf: user_libc.o start.o test.o  
	$(CXX) $(CXXFLAGS) user_libc.o start.o test.o -o test.elf
user_libc.o: user_libc.cc
start.o: start.s
test.o: test.cc

.PHONY: clean
clean:
	$(RM) *.o *.elf
