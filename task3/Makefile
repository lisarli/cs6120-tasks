.PHONY: tool dce test clean

dce_build: dce

dce: dce.cpp
	g++ -std=c++11 -o dce dce.cpp

test: dce_build
	turnt dce_test/*.bril

clean:
	rm -f dce