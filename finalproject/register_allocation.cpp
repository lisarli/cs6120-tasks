#include <algorithm>
#include <fstream>
#include <iostream>
#include <ostream>
#include <unordered_set>

#include "../task4/dataflow_utils.hpp"
#include <vector>


std::vector<std::set<std::string>> per_block_live_vars(const Block& block, std::set<std::string>& live_vars) {
    std::vector<std::set<std::string>> live_vars_per_instr(block.size());
    std::cout << block;
    for (int i = block.size() - 1; i >= 0; i--) {
        const auto& instr = block[i];
        live_vars_per_instr[i] = live_vars;
        if (instr.contains("dest")) {
            live_vars.erase(instr["dest"].get<std::string>());
        }
        if (instr.contains("args")) {
            for (const auto& arg : instr["args"]) {
                live_vars.insert(arg.get<std::string>());
            }
        }
    }
    return live_vars_per_instr;
}

std::pair<std::unordered_map<std::string, int>, int> linear_scan(std::vector<std::set<std::string>> live_vars, int num_registers) {
    std::vector<int> free_registers;
    for (int i = num_registers; i > 0; i--) {
        free_registers.push_back(i);
    }

    auto intervals = std::unordered_map<std::string, std::pair<int, int>>(); // map of var to (start, end)
    for (int i = 0; i < live_vars.size(); i++) {
        for (const auto& var : live_vars[i]) {
            if (intervals.find(var) == intervals.end()) {
                intervals[var] = {i, i};
            } else {
                intervals[var].second = i;
            }
        }
    }

    // std::cout << "intervals: " << std::endl;
    // for (const auto& interval : intervals) {
    //     std::cout << interval.first << ": " << interval.second.first << " " << interval.second.second << std::endl;
    // }

    // sort intervals by start time
    std::vector<std::pair<std::string, std::pair<int, int>>> sorted_intervals(intervals.begin(), intervals.end());
    std::sort(sorted_intervals.begin(), sorted_intervals.end(),
              [](const auto& a, const auto& b) { return a.second.first < b.second.first; });

    std::unordered_set<std::string> active_intervals;

    // heap for expiring intervals sorted in increasing endpoint
    auto cmp = [](const std::pair<std::string, std::pair<int, int>>& a,
                  const std::pair<std::string, std::pair<int, int>>& b) {
        return a.second.second > b.second.second;
    };
    std::priority_queue<std::pair<std::string, std::pair<int, int>>, std::vector<std::pair<std::string, std::pair<int, int>>>, decltype(cmp)> expiring_intervals(cmp);
    std::unordered_map<std::string, int> var_to_reg; 
    int spilled_vars = 0;
    
    for (const auto& interval : sorted_intervals) {
        const auto& var = interval.first;
        const auto& start = interval.second.first;
        const auto& end = interval.second.second;

        // expire old intervals
        while (!expiring_intervals.empty() && expiring_intervals.top().second.second < start) {
            active_intervals.erase(expiring_intervals.top().first);
            free_registers.push_back(var_to_reg[expiring_intervals.top().first]);
            expiring_intervals.pop();
        }

        if (active_intervals.size() == num_registers) {
            const auto& spill = expiring_intervals.top();
            if (spill.second.second > end) {
                var_to_reg[var] = var_to_reg[spill.first];
                var_to_reg[spill.first] = -1; // mark as spilled
                spilled_vars++;
                active_intervals.erase(spill.first);
                expiring_intervals.pop();
                active_intervals.insert(var);
                expiring_intervals.push(interval);
            } else {
                var_to_reg[var] = -1;
                spilled_vars++;
            }
            
        } else {
            int reg = free_registers.back();
            free_registers.pop_back();
            var_to_reg[var] = reg;
            active_intervals.insert(var);
            expiring_intervals.push(interval);
        }
    }

    return make_pair(var_to_reg, spilled_vars);
}

void alloc_register_linear_scan(json& func, int num_registers) {
    Cfg cfg = get_cfg_func(func);
    std::vector<Block> blocks = cfg.blocks;
    auto live_vars = df_live_vars(func, false).first; // get all outs of blocks for live var

    for (int i = 0; i < blocks.size(); i++) {
        auto& block = blocks[i];
        auto& block_live_vars = live_vars[i];

        auto live_vars_per_instr = per_block_live_vars(block, block_live_vars);
        // for (int j = 0; j < live_vars_per_instr.size(); j++) {
        //     std::cout << std::endl;
        //     if (live_vars_per_instr[j].empty()) {
        //         std::cout << "block " << i << " instr " << j << ": empty" << std::endl;
        //     } else {
        //         std::cout << "block " << i << " instr " << j << ": ";
        //         for (const auto& var : live_vars_per_instr[j]) {
        //             std::cout << var << " ";
        //         }
        //         std::cout << std::endl;
        //     }
        // }
        auto ls = linear_scan(live_vars_per_instr, num_registers);
        auto& var_to_reg = ls.first;
        auto& spilled_vars = ls.second;

        // print var to reg
        std::cout << "var to reg: " << std::endl;
        for (const auto& var : var_to_reg) {
            std::cout << var.first << ": " << var.second << std::endl;
        }
        std::cout << "spilled vars: " << spilled_vars << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // get utility type
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << "<linear|ssa>" << "<# registers>" << std::endl;
        return 1;
    }
    std::string utility_type = argv[1];
    
    int num_registers;
    try {
        num_registers = std::stoi(argv[2]);
    } catch (const std::invalid_argument& e) {
        std::cerr << "ERROR: Invalid number of registers: " << argv[2] << std::endl;
        return 1;
    }

    json j;
    try {
        std::cin >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse JSON from stdin, " << e.what() << std::endl;
        return 1;
    }
    
    for(auto& func: j["functions"]){
        if (utility_type == "linear") {
            alloc_register_linear_scan(func, num_registers);
        } else {
            std::cerr << "ERROR: Unknown utility type, got " << utility_type << std::endl;
            return 1;
        }
    }

    // std::cout << j;

    return 0;
}
