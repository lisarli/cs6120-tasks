.PHONY: tool test clean brench

# --- dce ---
dce_build: dce

dce: dce.cpp ../task2/cfg/cfg_utils.cpp
	g++ -std=c++11 -o dce dce.cpp ../task2/cfg/cfg_utils.cpp

test_dce: dce_build
	turnt dce_test/*.bril -e dce

brench_dce: dce_build
	brench dce_brench.toml

# --- lvn ---
lvn_build: lvn

lvn: lvn.cpp ../task2/cfg/cfg_utils.cpp
	g++ -std=c++20 -o lvn lvn.cpp ../task2/cfg/cfg_utils.cpp

test_lvn: lvn_build
	turnt lvn_test/*.bril -e lvn

brench_lvn: lvn_build
	brench lvn_brench.toml

# --- bash shortcuts ---
check_output:
	@for file in $(TEST_DIR)/*.bril; do \
		bril2json < "$$file" | brili > out1.txt; \
		bril2json < "$$file" | ./$(EXEC) | brili > out2.txt; \
		if diff -q out1.txt out2.txt > /dev/null; then \
			echo "$$file: match"; \
		else \
			echo "$$file: MISMATCH"; \
			diff out1.txt out2.txt; \
		fi; \
	done; \
	rm -f out1.txt out2.txt

clean:
	rm -f dce lvn