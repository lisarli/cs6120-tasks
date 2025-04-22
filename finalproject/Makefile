.PHONY: clean 

register_allocation: register_allocation.cpp ../task2/cfg/cfg_utils.hpp ../task2/cfg/cfg_utils.cpp ../task4/dataflow_utils.hpp ../task4/dataflow_utils.cpp
	g++ -Wall -std=c++20 -I /opt/homebrew/include -o register_allocation register_allocation.cpp ../task2/cfg/cfg_utils.cpp ../task4/dataflow_utils.cpp

clean:
	rm -f register_allocation