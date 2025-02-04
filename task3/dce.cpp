#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include <cassert>
#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg.hpp"


// perform one tdce pass, return whether any change found
bool tdce_one_pass(json& func){
    // find all used vars
    std::set<std::string> seen;
    for(const auto& instr: func["instrs"]){
        assert(!instr.contains("label"));
        if(instr.contains("args")){
            for(auto& arg: instr["args"]){
                seen.insert(arg);
            }
        }
    }

    // remove any unused assigns
    bool found_opt = false;
    auto& instrs = func["instrs"];
    for(auto it = instrs.begin(); it != instrs.end(); ){
        if (it->contains("dest") && !seen.count((*it)["dest"])) {
            found_opt = true;
            it = instrs.erase(it);
        } else {
            ++it; 
        }
    }
    return found_opt;
}

// tdce with iteration to convergence
void tdce(json& func){
    bool changed = true;
    while(changed){
        changed = tdce_one_pass(func);
    }
}

int main() {
    // open bril json
    json j;
    try {
        std::cin >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse JSON from stdin: " << e.what() << std::endl;
        return 1;
    }
    
    // perform trivial dead code elimination
    for(auto& func: j["functions"]){
        tdce(func);
        std::cout << j << std::endl;
    }

    return 0;
}
