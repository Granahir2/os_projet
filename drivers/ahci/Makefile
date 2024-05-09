CXXFLAGS += -O0
ahci.o: ahci_interf.o command.o interface.o
	$(CXX) $(CXXFLAGS) -r $^ -o ahci.o

interface.o: interface.cc interface.hh ahci.hh 
	$(CXX) $(CXXFLAGS) -c interface.cc -o interface.o

ahci_interf.o: ahci.hh ahci.cc hba_defs.hh
	$(CXX) $(CXXFLAGS) -c ahci.cc -o ahci_interf.o

command.o: hl_command.cc hl_command.hh hba_defs.hh
	$(CXX) $(CXXFLAGS) -c hl_command.cc -o command.o


.PHONY: clean
clean:
	$(RM) *.o
