#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unordered_map>
#include <queue>
#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"


std::set<std::string> get_defs(const Block& b){
    std::set<std::string> defs;
    for(auto& instr: b){
        if(instr.contains("dest")){
            defs.insert(instr["dest"]);
        }
    }
    return defs;
}

// performing reaching definitions dataflow analysis
void df_defined_vars(const json& func){
    Cfg cfg = get_cfg_func(func);
    std::vector<Block>& blocks = cfg.blocks;

    // create init
    std::set<std::string> init;
    if(func.contains("args")){
        for(auto arg: func["args"]){
            init.insert(arg["name"]);
        }
    }

    // initialize in[entry] and out[*]
    std::unordered_map<int,std::set<std::string>> in;
    std::unordered_map<int,std::set<std::string>> out;
    in[cfg.entryIdx] = init;
    for(int i = 0; i < blocks.size(); i++){
        out[i] = init;
    }

    // queue all blocks in worklist
    std::queue<int> worklist;
    for(int i = 0; i < blocks.size(); i++){
        worklist.push(i);
    }

    while(!worklist.empty()){
        int b = worklist.front(); worklist.pop();
        auto& in_b = in[b];

        // merge from predecessors
        for(int p: cfg.preds[b]){
            in_b.insert(out[p].begin(), out[p].end());
        }

        // transfer through block
        int prev_size = out[b].size();
        auto defs = get_defs(blocks[b]);
        defs.insert(in_b.begin(), in_b.end());
        out[b] = defs;

        // queue successors if changed
        if(defs.size() != prev_size){
            for(int s: cfg.succs[b]){
                worklist.push(s);
            }
        }
    }

    // display defined variables
    for(int i = 0; i < blocks.size(); i++){
        std::cout << "\tBlock " << i << std::endl;
        std::cout << "\t\tin: ";
        for(auto var: in[i]){
            std::cout << var << " ";
        }
        std::cout << std::endl;
        std::cout << "\t\tout: ";
        for(auto var: out[i]){
            std::cout << var << " ";
        }
        std::cout << std::endl;
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
    
    // do dataflow analysis
    for(auto& func: j["functions"]){
        std::cout << "analyzing func: " << func["name"] << std::endl;
        df_defined_vars(func);
        std::cout << std::endl;
    }

    return 0;
}