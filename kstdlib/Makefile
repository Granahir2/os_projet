export CXXFLAGS+= -m64 -O3
MODS=cstring.o cstdio.o

.PHONY: clean
stdlib.o: $(MODS)
	g++ $(CXXFLAGS) $^ -r -o stdlib.o

cstring.o: cstring.cc cstring.hh
cstdio.o: cstring.cc cstring.hh cstdio.hh ../kernel/fs.hh
clean:
	$(RM) *.o
