.PHONY: tool test clean brench

all: dce_build lvn_build

# --- dce ---
dce_build: dce

dce: dce.cpp ../task2/cfg/cfg_utils.cpp
	g++ -std=c++20 -I /opt/homebrew/include -o dce dce.cpp ../task2/cfg/cfg_utils.cpp

test_dce: dce_build
	turnt dce_test/*.bril -e dce

brench_dce: dce_build
	brench dce_brench.toml

# --- lvn ---
lvn_build: lvn

lvn: lvn.cpp ../task2/cfg/cfg_utils.cpp
	g++ -std=c++20 -I /opt/homebrew/include  -o lvn lvn.cpp ../task2/cfg/cfg_utils.cpp

test_lvn: dce_build lvn_build
	turnt lvn_test/*.bril -e lvn_out

brench_lvn: dce_build lvn_build
	brench lvn_brench.toml

# --- clean ---
clean:
	rm -f dce lvn