#include "cfg.hpp"

std::vector<std::vector<json>> get_blocks(const json& func){
    std::vector<std::vector<json>> blocks;
    std::vector<json> cur_block;
    const std::set<std::string> terms = {"br","jmp","ret"};

    for(const auto& instr: func["instrs"]){
        bool is_label = instr.contains("label");
        bool is_term = !is_label && terms.count(instr["op"]);
        if(!is_label){
            cur_block.push_back(instr);
        }

        if(is_label || is_term){
            if(!cur_block.empty()){
                blocks.push_back(cur_block);
                cur_block.clear();   
            }
        }

        // remove labels from basic blocks
        // if(is_label){
        //     cur_block.push_back(instr);
        // }
    }

    if(!cur_block.empty()){
        blocks.push_back(cur_block);
    }

    return blocks;
}

std::map<std::string,int> get_label_ids(std::vector<std::vector<json>> bb){
    std::map<std::string,int> labels_to_id;
    for(int i = 0; i < bb.size(); i++){
        auto& block = bb[i];
        if(block[0].contains("label")){
            labels_to_id[block[0]["label"]] = i;
        }
    }
    return labels_to_id;
}


std::map<int,std::vector<int>> get_cfg(std::vector<std::vector<json>> bb){
    auto labels_to_id = get_label_ids(bb);
    std::map<int,std::vector<int>> cfg;

    for(int i = 0; i < bb.size(); i++){
        auto& block = bb[i];
        auto& last = block[block.size()-1];
        std::vector<int> succ;
        
        if(last["op"] == "jmp"){
            succ.push_back(labels_to_id[last["labels"][0]]);
        } else if(last["op"] == "br"){
            succ.push_back(labels_to_id[last["labels"][0]]);
            succ.push_back(labels_to_id[last["labels"][1]]);
        } else if(last["op"] != "ret" && i != bb.size()-1){
            succ.push_back(i+1);
        }

        cfg[i] = succ;
    }

    return cfg;
}

void print_bb(std::vector<std::vector<json>> bb){
    std::cout << "\t--- BB ---"  << std::endl;
    for(int i = 0; i < bb.size(); i++){
        std::cout << "\tblock " << i << std::endl;
        for(const auto& instr: bb[i]){
            std::cout << "\t\t" << instr << std::endl;
        }
    }
}

void print_cfg(std::map<int,std::vector<int>> cfg){
    std::cout << "\t--- CFG ---"  << std::endl;
    for(const auto& pair: cfg){
        std::cout << "\t" << pair.first << " goes to ";
        for(const auto& cur: pair.second){
            std::cout << cur << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
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
    
    for(const auto& func: j["functions"]){
        std::cout << "function: " << func["name"] << std::endl;
        std::cout << "\targs: " << func["args"] << std::endl;

        auto bb = get_blocks(func);
        print_bb(bb);
        auto cfg = get_cfg(bb);
        print_cfg(cfg);
    }

    return 0;
}
