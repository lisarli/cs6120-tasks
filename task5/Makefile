.PHONY: clean dom_build

dom_build: dom.cpp ../task2/cfg/cfg_utils.cpp
	g++ -std=c++20 -I /opt/homebrew/include -o dom dom.cpp ../task2/cfg/cfg_utils.cpp

clean:
	rm -f dom