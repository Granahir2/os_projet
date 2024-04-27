fat.o: fat_fs.o fat_dir_iterator.o fat_file.o
	$(CXX) $(CXXFLAGS) -r $^ -o fat.o
fat.hh: ../../kstdlib/string.hh
fat_fs.o: fat_fs.cc fat.hh
	$(CXX) -c $(CXXFLAGS) fat_fs.cc -o fat_fs.o
fat_dir_iterator.o: fat_dir_iterator.cc fat.hh
	$(CXX) -c $(CXXFLAGS) fat_dir_iterator.cc -o fat_dir_iterator.o
fat_file.o: fat_file.cc fat.hh
	$(CXX) -c $(CXXFLAGS) fat_file.cc -o fat_file.o
.PHONY: clean
clean:
	$(RM) *.o

