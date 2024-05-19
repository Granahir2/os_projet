export CXXFLAGS+= -m64 -O3
MODS=cstring.o cstdio.o new.o cstdlib.o

.PHONY: clean
stdlib.o: $(MODS)
	$(CXX) $(CXXFLAGS) $^ -r -o stdlib.o

cstring.o: cstring.cc cstring.hh
cstdlib.o: cstdlib.hh cstdlib.cc
new.o: new.cc new.hh ../kernel/kernel.hh
cstdio.o: cstdio.cc cstring.hh cstdio.hh ../kernel/fs/fs.hh
map.o: map.cc map.hh
clean:
	$(RM) *.o
