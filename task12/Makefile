.PHONY: clean 

trace: trace.cpp ../task2/cfg/cfg_utils.hpp ../task2/cfg/cfg_utils.cpp ../task5/dom_utils.hpp ../task5/dom_utils.cpp
	g++ -std=c++20 -I /opt/homebrew/include -o trace trace.cpp ../task2/cfg/cfg_utils.cpp ../task5/dom_utils.cpp

clean:
	rm -f trace