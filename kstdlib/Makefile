export CXXFLAGS+= -m64 -O3
MODS=cstring.o cstdio.o new.o

.PHONY: clean
stdlib.o: $(MODS)
	g++ $(CXXFLAGS) $^ -r -o stdlib.o

cstring.o: cstring.cc cstring.hh
new.o: new.cc new.hh ../kernel/kernel.hh
cstdio.o: cstdio.cc cstring.hh cstdio.hh ../kernel/fs.hh
clean:
	$(RM) *.o
