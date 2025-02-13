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

template<typename T, typename M, typename R, typename D>
void df_worklist(const json& func, bool is_forward, T init, M merge, R transfer, D display){
    // get CFG
    Cfg cfg = get_cfg_func(func);
    std::vector<Block>& blocks = cfg.blocks;

    // set direction
    auto preds = cfg.preds;
    auto succs = cfg.succs;
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
        bool changed = transfer(blocks[b], out[b], in[b], b);

        // queue successors if changed
        if(changed){
            for(int s: succs[b]){
                worklist.push(s);
            }
        }
    }
    
    // display analysis
    display(blocks, in, out);
}

// defined vars df analysis
void df_defined_vars(const json& func){
    // create init
    std::set<std::string> init;
    // if(func.contains("args")){
    //     for(auto arg: func["args"]){
    //         init.insert(arg["name"]);
    //     }
    // }

    // create merge
    auto merge = [](std::set<std::string>& in_b, const std::set<std::string>& pred){
        in_b.insert(pred.begin(), pred.end());
    };

    // create transfer
    auto transfer = [](const Block& block, std::set<std::string>& out_b, const std::set<std::string>& in_b, int b){
        int prev_size = out_b.size();
        auto defs = get_defs(block);
        defs.insert(in_b.begin(), in_b.end());
        out_b = std::move(defs);
        return prev_size != out_b.size();
    };

    // create display
    auto display = [](std::vector<Block>& blocks, std::unordered_map<int,std::set<std::string>>& in, std::unordered_map<int,std::set<std::string>>& out){
        int empty_block_counter = 1;
        for(int i = 0; i < blocks.size(); i++){
            auto label = "b" + std::to_string(empty_block_counter);
            if (blocks[i][0].contains("label")) {
                label = blocks[i][0]["label"];
            } else {
                empty_block_counter++;
            }
            std::cout << label << ":" << std::endl;
            std::cout << "  in:  ";
            if (in[i].empty()) {
                std::cout << "∅";
            } else {
                for (auto iter = in[i].begin(); iter != in[i].end(); iter++) {
                    if (iter != in[i].begin()) std::cout << ", ";
                    std::cout << *iter;
                }
            }
            std::cout << std::endl;
            std::cout << "  out: ";
            if (out[i].empty()) {
                std::cout << "∅";
            } else {
                for (auto iter = out[i].begin(); iter != out[i].end(); iter++) {
                    if (iter != out[i].begin()) std::cout << ", ";
                    std::cout << *iter;
                }
            }
            std::cout << std::endl;
        }
    };

    df_worklist(func, true, init, merge, transfer, display);
}

// reaching defs df analysis
void df_reaching_defs(const json& func){
    // map var names to locations of all reaching defs
    using DefStore = std::unordered_map<std::string,std::set<std::string>>;

    // create init
    DefStore init;

    // create merge
    auto merge = [](DefStore& in_b, const DefStore& pred){
        for(const auto& cur: pred){
            auto& prev_defs = cur.second;
            in_b[cur.first].insert(prev_defs.begin(), prev_defs.end());
        }
    };

    // create transfer
    auto transfer = [](const Block& block, DefStore& out_b, const DefStore& in_b, int b){
        auto prev_out_b = out_b;
        out_b = in_b;
        std::string block_id = "b" + std::to_string(b) + ".";
        for(int i = 0; i < block.size(); i++){
            const auto& instr = block[i];
            if(instr.contains("dest")){
                out_b[instr["dest"]] = {block_id + std::to_string(i)};
            }
        }
        return prev_out_b != out_b;
    };

    // create display
    auto display = [](const std::vector<Block>& blocks, std::unordered_map<int,DefStore>& in, std::unordered_map<int,DefStore>& out){
        for(int i = 0; i < blocks.size(); i++){
            std::cout << "b" << i << std::endl;
            
            // display in
            std::cout << "  in:  " << std::endl;
            for(const auto& cur: in[i]){
                std::cout << "    " << cur.first << ":  ";
                for(const auto& def: cur.second){
                    std::cout << def << " ";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
            
            // display out
            std::cout << "  out:  " << std::endl;
            for(const auto& cur: out[i]){
                std::cout << "    " << cur.first << ":  ";
                for(const auto& def: cur.second){
                    std::cout << def << " ";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    };

    df_worklist(func, true, init, merge, transfer, display);
}

// live vars df analysis
void df_live_vars(const json& func){
    // create init
    std::set<std::string> init;

    // create merge
    auto merge = [](std::set<std::string>& in_b, const std::set<std::string>& pred){
        in_b.insert(pred.begin(), pred.end());
    };

    // create transfer
    auto transfer = [](const Block& block, std::set<std::string>& out_b, const std::set<std::string>& in_b, int b){
        std::set<std::string> out_new(in_b);

        for(int i = block.size()-1; i >= 0; i--){
            const json& instr = block[i];
            // removed killed
            std::string dest = "";
            if(instr.contains("dest")){
                out_new.erase(instr["dest"]);
            }

            // add uses
            if(instr.contains("args")){
                for(auto arg: instr["args"]){
                    if(!out_new.contains(arg)){
                        out_new.insert(arg);
                    }
                }
            }
        }

        bool changed = out_new != out_b;
        out_b = std::move(out_new);
        return changed;
    };

    // create display
    auto display = [](std::vector<Block>& blocks, std::unordered_map<int,std::set<std::string>>& in, std::unordered_map<int,std::set<std::string>>& out){
        int empty_block_counter = 1;
        for(int i = 0; i < blocks.size(); i++){
            auto label = "b" + std::to_string(empty_block_counter);
            if (blocks[i][0].contains("label")) {
                label = blocks[i][0]["label"];
            } else {
                empty_block_counter++;
            }
            std::cout << label << ":" << std::endl;
            std::cout << "  in:  ";
            if (out[i].empty()) {
                std::cout << "∅";
            } else {
                for (auto iter = out[i].begin(); iter != out[i].end(); iter++) {
                    if (iter != out[i].begin()) std::cout << ", ";
                    std::cout << *iter;
                }
            }
            std::cout << std::endl;
            std::cout << "  out: ";
            if (in[i].empty()) {
                std::cout << "∅";
            } else {
                for (auto iter = in[i].begin(); iter != in[i].end(); iter++) {
                    if (iter != in[i].begin()) std::cout << ", ";
                    std::cout << *iter;
                }
            }
            std::cout << std::endl;
        }
    };

    df_worklist(func, false, init, merge, transfer, display);
}

int main(int argc, char* argv[]) {
    // get df type
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <defined|live|reaching>" << std::endl;
        return 1;
    }
    std::string df_type = argv[1];

    // open bril json
    json j;
    try {
        std::cin >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse JSON from stdin, " << e.what() << std::endl;
        return 1;
    }
    
    // do analysis
    for(auto& func: j["functions"]){
        // std::cout << "analyzing func: " << func["name"] << std::endl;
        if(df_type == "defined"){
            df_defined_vars(func);
        } else if(df_type == "live"){
            df_live_vars(func);
        } else if(df_type == "reaching"){
            df_reaching_defs(func);
        } else{
            std::cout << "ERROR: Unknown df type, got " << df_type << std::endl;
            return 1;
        }
        std::cout << std::endl;
    }

    return 0;
}