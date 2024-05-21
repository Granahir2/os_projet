scheduling.o: proc.o scheduler_main.o saveregs_hook.o sched.o 
	$(CXX) $(CXXFLAGS) -r $^ -o scheduling.o  
proc.o: proc.cc proc.hh
	$(CXX) -c $(CXXFLAGS) $< -o $@
scheduler_main.o: scheduler_main.cc scheduler_main.hh
	$(CXX) -c $(CXXFLAGS) $< -o $@
saveregs_hook.o: saveregs_hook.s
	$(CXX) -c $(CXXFLAGS) $< -o $@
sched.o: sched.cpp sched.hh
	$(CXX) -c $(CXXFLAGS) $< -o $@
