#include <algorithm>
#include <fstream>
#include <iostream>

#include "../task4/dataflow_utils.hpp"
#include <ostream>

void linear_scan(json& func) {
    Cfg cfg = get_cfg_func(func);
    std::vector<Block> blocks = cfg.blocks;
    auto live_vars = df_live_vars(func, false).first; // get all outs of blocks for live var

    // std::cout << "live vars: " << std::endl;
    // for (const auto& var : live_vars) {
    //     std::cout << var.first << ": ";
    //     for (const auto& v : var.second) {
    //         std::cout << v << " ";
    //     }
    //     std::cout << std::endl;
    // }
}

int main(int argc, char* argv[]) {
    // get utility type
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << "<linear|ssa>" << std::endl;
        return 1;
    }
    std::string utility_type = argv[1];

    json j;
    try {
        std::cin >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse JSON from stdin, " << e.what() << std::endl;
        return 1;
    }
    
    for(auto& func: j["functions"]){
        if (utility_type == "linear") {
            linear_scan(func);
        } else {
            std::cerr << "ERROR: Unknown utility type, got " << utility_type << std::endl;
            return 1;
        }
    }

    // std::cout << j;

    return 0;
}
