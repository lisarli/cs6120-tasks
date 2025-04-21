#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <stack>
#include <map>

#include "../task4/dataflow.hpp"

void linear_scan(json& func) {
    Cfg cfg = get_cfg_func(func);
    std::vector<Block> blocks = cfg.blocks;
    auto live_vars = df_live_vars(func);
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

    std::cout << j;

    return 0;
}
