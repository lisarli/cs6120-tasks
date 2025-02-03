.PHONY: tool test clean

tool: count_int_ops

count_int_ops: count_int_ops.cpp
	g++ -std=c++11 -o count_int_ops count_int_ops.cpp

test: tool
	turnt tool_test/*.bril

clean:
	rm -f count_int_ops