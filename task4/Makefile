.PHONY: clean df_build

df_build: dataflow.cpp ../task2/cfg/cfg_utils.cpp
	g++ -std=c++20 -I /opt/homebrew/include -o df dataflow.cpp ../task2/cfg/cfg_utils.cpp

clean:
	rm -f df