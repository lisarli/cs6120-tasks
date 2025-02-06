.PHONY: tool test clean count_int_ops cfg_build

tool: count_int_ops

count_int_ops: tool/count_int_ops.cpp
	g++ -std=c++11 -o tool/count_int_ops tool/count_int_ops.cpp

test: tool
	turnt tool/tool_test/*.bril

cfg_build: cfg/cfg.cpp cfg/cfg_utils.cpp
	g++ -std=c++11 -o cfg/cfg cfg/cfg.cpp cfg/cfg_utils.cpp

clean:
	rm -f tool/count_int_ops cfg/cfg dce/dce