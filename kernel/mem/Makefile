utils.o: utils.cc utils.hh ../../x86/asm_stubs.hh
	$(CXX) $(CXXFLAGS) -c utils.cc -o utils.o 
phmem_manager.o: utils.hh phmem_manager.hh phmem_manager.cc ../../x86/asm_stubs.hh ../kernel.hh
	$(CXX) $(CXXFLAGS) -c phmem_manager.cc -o phmem_manager.o 

.PHONY: clean
	$(RM) *.o	
