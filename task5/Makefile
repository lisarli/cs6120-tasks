.PHONY: clean dom_build

dom_build: dom.cpp ../task2/cfg/cfg_utils.cpp dom_utils.cpp dom_utils.hpp
	g++ -std=c++20 -I /opt/homebrew/include -o dom dom.cpp ../task2/cfg/cfg_utils.cpp dom_utils.cpp

clean:
	rm -f dom