#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unordered_map>
#include <queue>
#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"


// get names of variables written to in block [b]
std::set<std::string> get_defs(const Block& b){
    std::set<std::string> defs;
    for(auto& instr: b){
        if(instr.contains("dest")){
            defs.insert(instr["dest"]);
        }
    }
    return defs;
}

// perform defined vars dataflow analysis
template<typename T, typename M, typename R, typename D>
void df_worklist(const json& func, bool is_forward, T init, M merge, R transfer, D display){
    // get CFG
    Cfg cfg = get_cfg_func(func);
    std::vector<Block>& blocks = cfg.blocks;

    // set direction
    auto& preds = cfg.preds;
    auto& succs = cfg.succs;
    if(!is_forward){
        succs = cfg.preds;
        preds = cfg.succs;
    }

    // initialize in[entry] and out[*]
    std::unordered_map<int,T> in;
    std::unordered_map<int,T> out;
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

        // merge from predecessors
        for(int p: preds[b]){
            merge(in[b], out[p]);
        }

        // transfer through block
        bool changed = transfer(blocks[b], out[b], in[b]);
        

        // queue successors if changed
        if(changed){
            for(int s: succs[b]){
                worklist.push(s);
            }
        }
    }
    
    // display analysis
    display(blocks.size(), in, out);
}

void df_defined_vars(const json& func){
    // create init
    std::set<std::string> init;
    if(func.contains("args")){
        for(auto arg: func["args"]){
            init.insert(arg["name"]);
        }
    }

    // create merge
    auto merge = [](std::set<std::string>& in_b, const std::set<std::string>& pred){
        in_b.insert(pred.begin(), pred.end());
    };

    // create transfer
    auto transfer = [](const Block& block, std::set<std::string>& out_b, const std::set<std::string>& in_b){
        int prev_size = out_b.size();
        auto defs = get_defs(block);
        defs.insert(in_b.begin(), in_b.end());
        out_b = std::move(defs);
        return prev_size != out_b.size();
    };

    // create display
    auto display = [](int num_blocks, std::unordered_map<int,std::set<std::string>>& in, std::unordered_map<int,std::set<std::string>>& out){
        for(int i = 0; i < num_blocks; i++){
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
    };

    df_worklist(func, true, init, merge, transfer, display);
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