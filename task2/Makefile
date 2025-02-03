.PHONY: tool test clean

tool: count_int_ops.cpp
	g++ -std=c++11 -o count_int_ops count_int_ops.cpp

test: count_int_ops
	turnt tool_test/*.bril

clean:
	rm -f count_int_ops