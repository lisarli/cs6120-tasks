#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <set>
#include <map>
#include <utility>
#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"

// one pass to remove unused var names
bool remove_unused_var(json& func){
    // find all used vars
    std::set<std::string> seen;
    for(const auto& instr: func["instrs"]){
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

// one pass to remove values overwritten before read
bool remove_local_killed(json& func){
    bool found_opt = false;

    std::vector<Block> blocks = get_blocks(func);
    for(auto& b: blocks){
        std::set<int> invalid;
        std::map<std::string,std::pair<int,bool>> usages;
        int num_instrs_start = b.size();

        for(int i = 0; i < num_instrs_start; i++){
            auto& instr = b[i];
            
            // process usages (args)
            if(instr.contains("args")){
                for(auto& arg: instr["args"]){
                    if(usages.count(arg)){
                        usages[arg].second = true;
                    }
                }
            }

            // write new dest usage
            if(instr.contains("dest")){
                std::string dest = instr["dest"];
                if(usages.count(dest) && !usages[dest].second){
                    invalid.insert(usages[dest].first);
                }
                usages[dest] = std::make_pair(i, false);
            }
        }

        // remove overwritten instructions
        auto it = b.begin();
        for(int i = 0; i < num_instrs_start; i++){
            if(invalid.count(i)){
                it = b.erase(it);
            } else{
                it++;
            }
        }
        found_opt = !invalid.empty();
    }
    
    // update function body
    std::vector<json> new_func_body;
    for(const auto& b: blocks){
        new_func_body.insert(new_func_body.end(), b.begin(), b.end());
    }
    func["instrs"] = new_func_body;

    return found_opt;
}

// tdce with iteration to convergence
void tdce(json& func){
    bool changed = true;
    while(changed){
        changed = remove_unused_var(func) || remove_local_killed(func);
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
    }

    // output new program
    std::cout << j << std::endl;

    return 0;
}
