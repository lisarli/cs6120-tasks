#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"


using Dom = std::unordered_map<int,std::set<int>>;

// get postorder traversal order
void get_post_order(const Cfg& cfg, std::vector<int>& order, int cur_node, std::set<int>& visited){
    if(visited.contains(cur_node)) return;
    visited.insert(cur_node);

    if(cfg.succs.contains(cur_node)){
        for(int succ: cfg.succs.at(cur_node)){
            get_post_order(cfg, order, succ, visited);
        }
    }
    order.push_back(cur_node);
}

// get reversed postorder traversal order
std::vector<int> get_rev_post_order(const Cfg& cfg){
    std::vector<int> order;
    std::set<int> visited;
    get_post_order(cfg, order, cfg.entryIdx, visited);
    std::reverse(order.begin(), order.end());
    return order;
}

// get map of nodes to dominators
Dom get_dom(const json& func){
    Dom dom;

    // get CFG
    Cfg cfg = get_cfg_func(func);
    std::vector<Block>& blocks = cfg.blocks;

    // init dom[every block] -> all blocks
    std::set<int> init;
    for(int i = 0; i < blocks.size(); i++){
        init.insert(i);
    } 
    for(int i = 0; i < blocks.size(); i++){
        dom[i] = init;
    }

    // dom[entry] = entry
    dom[cfg.entryIdx] = {cfg.entryIdx};

    // get reverse postorder order
    const auto& order = get_rev_post_order(cfg);

    // iterate to convergence
    bool changed = true;
    while(changed){
        changed = false;

        // iterate over nodes
        for(int i = 0; i < order.size(); i++){   
            if(i == cfg.entryIdx) continue;   
            auto& preds = cfg.preds.at(i);
            auto dom_new = dom[preds[0]];

            // get union over preds
            for(int j = 1; j < preds.size(); j++){
                int p = preds[j];
                std::set<int> temp;
                std::set_intersection(dom_new.begin(), dom_new.end(),
                                      dom[p].begin(), dom[p].end(),
                                      std::inserter(temp, temp.end()));
                dom_new = std::move(temp);
            }

            // insert self
            dom_new.insert(i);
            
            // update dom[self]
            if(dom_new != dom[i]) changed = true;
            dom[i] = dom_new;
        }
    }

    // display
    for(const auto& cur: dom){
        std::cout << get_block_name(cfg, cur.first) << ": ";
        for(auto dominator: cur.second){
            std::cout << get_block_name(cfg, dominator) << " ";
        }
        std::cout << std::endl;
    }

    return dom;
}

int main(int argc, char* argv[]) {
    // get utility type
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <dom|tree|frontier>" << std::endl;
        return 1;
    }
    std::string utility_type = argv[1];

    // open bril json
    json j;
    try {
        std::cin >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse JSON from stdin, " << e.what() << std::endl;
        return 1;
    }
    
    // do analysis
    auto& func = j["functions"][0]; // assume just main

    if(utility_type == "dom"){
        get_dom(func);
    } else{
        std::cout << "ERROR: Unknown df type, got " << utility_type << std::endl;
        return 1;
    }

    return 0;
}