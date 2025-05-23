#include "cfg_utils.hpp"

std::vector<Block> get_blocks(const json& func){
    std::vector<Block> blocks;
    Block cur_block;
    const std::set<std::string> terms = {"br","jmp","ret"};
    
    bool got_ret = false;
    for(const auto& instr: func["instrs"]){
        bool is_label = instr.contains("label");
        bool is_term = !is_label && terms.count(instr["op"]);

        if(got_ret && !is_label) continue;
        if(is_term) got_ret = true;

        if(!is_label){
            cur_block.push_back(instr);
        } else{
            got_ret = false;
        }

        if(is_label || is_term){
            if(!cur_block.empty()){
                blocks.push_back(cur_block);
                cur_block.clear();
            }
        }

        if(is_label){
            cur_block.push_back(instr);
        }
    }

    if(!cur_block.empty()){
        blocks.push_back(cur_block);
    }

    return blocks;
}

std::map<std::string,int> get_label_ids(std::vector<Block> bb){
    std::map<std::string,int> labels_to_id;
    for(int i = 0; i < bb.size(); i++){
        auto& block = bb[i];
        if(block[0].contains("label")){
            labels_to_id[block[0]["label"]] = i;
        }
    }
    return labels_to_id;
}

Cfg get_cfg(std::vector<Block> bb){
    auto labels_to_id = get_label_ids(bb);
    Cfg cfg;
    cfg.blocks = bb;

    int i;
    for(i = 0; i < bb.size(); i++){
        // initialize with empty sets
        cfg.preds[i];
        cfg.succs[i];

        auto& block = bb[i];
        auto& last = block[block.size()-1];
        std::set<int> succ;
        
        if(last["op"] == "jmp"){
            succ.insert(labels_to_id[last["labels"][0]]);
        } else if(last["op"] == "br"){
            succ.insert(labels_to_id[last["labels"][0]]);
            succ.insert(labels_to_id[last["labels"][1]]);
        } else if(last["op"] != "ret" && i != bb.size()-1){
            succ.insert(i+1);
        }

        cfg.succs[i] = succ;

        for(int node: succ){
            cfg.preds[node].insert(i);
        }
    }

    // add empty entry block if first block has predecessor
    cfg.entryIdx = 0;
    if(!cfg.preds[0].empty()){
        Block emptyBlock;
        cfg.blocks.push_back(emptyBlock);
        cfg.succs[i].insert(0);
        cfg.preds[0].insert(i);
        cfg.entryIdx = i;
        cfg.preds[i];
    }

    // construct block order
    auto& block_order = cfg.block_order;
    for(int i = 0; i < cfg.blocks.size(); i++){
        block_order.push_back(i);
    }
    if(cfg.entryIdx != 0){
        block_order.insert(block_order.begin(), cfg.blocks.size()-1);
        block_order.pop_back();
    }

    return cfg;
}

Cfg get_cfg_func(const json& func){
    auto bb = get_blocks(func);
    return get_cfg(bb);
}

void print_bb(std::vector<Block> bb){
    std::cout << "\t--- BB ---"  << std::endl;
    for(int i = 0; i < bb.size(); i++){
        std::cout << "\tblock " << i << std::endl;
        for(const auto& instr: bb[i]){
            std::cout << "\t\t" << instr << std::endl;
        }
    }
}

void print_cfg(Cfg cfg){
    std::cout << "\t--- CFG ---"  << std::endl;
    for(const auto& pair: cfg.succs){
        std::cout << "\t" << get_block_name(cfg, pair.first) << " goes to ";
        for(const auto& cur: pair.second){
            std::cout << get_block_name(cfg, cur) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// get block name (label if it exists, entry if entry node)
std::string get_block_name(const Cfg& cfg, int block_idx){
    auto& block = cfg.blocks[block_idx];
    if(block.size() > 0 && block[0].contains("label")){
        return block[0]["label"];
    }
    if(block_idx == cfg.entryIdx){
        return "entry";
    }
    return "b" + std::to_string(block_idx);
}