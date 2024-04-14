CXXFLAGS += -m64 -mcmodel=large

crti.o: crti.s
	$(CXX) $(CXXFLAGS) -c $< -o $@ 
crtn.o: crtn.s
	$(CXX) $(CXXFLAGS) -c $< -o $@ 
crt0.o: crt0.s
	$(CXX) $(CXXFLAGS) -c $< -o $@ 
