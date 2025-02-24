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
using DomTree = std::unordered_map<int,std::set<int>>;

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
Dom get_dom(const json& func, bool verbose = false){
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

    if (!verbose) return dom;

    for(const auto& cur: dom){
        std::cout << get_block_name(cfg, cur.first) << ": ";
        for(auto dominator: cur.second){
            std::cout << get_block_name(cfg, dominator) << " ";
        }
        std::cout << std::endl;
    }

    return dom;
}

Dom get_inverse_dom(const Dom& dom){
    Dom inverse_dom;
    for(const auto& cur: dom){
        for(int dom_node: cur.second){
            inverse_dom[dom_node].insert(cur.first);
        }
    }
    return inverse_dom;
}

DomTree get_dom_tree(const json& func, const Dom& dom){
    DomTree dom_tree;
    Dom inverse_dom = get_inverse_dom(dom);

    Dom no_self_inverse_dom = inverse_dom;
    for(const auto& cur: no_self_inverse_dom){
        int node = cur.first;
        no_self_inverse_dom.at(node).erase(node);
    }

    // for each node find nodes strictly dominated by children of node
    Dom children_dom = no_self_inverse_dom;
    for(const auto& cur: children_dom){
        int node = cur.first;
        std::set<int> children;
        for(int child: cur.second){
            std::set<int> temp;
            std::set_union(children.begin(), children.end(),
                           no_self_inverse_dom.at(child).begin(), no_self_inverse_dom.at(child).end(),
                           std::inserter(temp, temp.end()));
            children = std::move(temp);
        }
        children_dom[node] = children;
    }

    // find domtree, filter nodes
    for(const auto& cur: no_self_inverse_dom){
        std::set<int> immediate;
        for(int node: cur.second){
            // if node not in children_dom
            if(!children_dom[cur.first].contains(node)){
                immediate.insert(node);
            }
        }
        dom_tree[cur.first] = immediate;
    }

    //display
    Cfg cfg = get_cfg_func(func);
    for(const auto& cur: dom_tree){
        std::cout << get_block_name(cfg, cur.first) << ": ";
        for(int child: cur.second){
            std::cout << get_block_name(cfg, child) << " ";
        }
        std::cout << std::endl;
    }

    return dom_tree;
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
        get_dom(func, true);
    } else if(utility_type == "tree"){
        Dom dom = get_dom(func);
        get_dom_tree(func, dom);
    } else{
        std::cout << "ERROR: Unknown df type, got " << utility_type << std::endl;
        return 1;
    }

    return 0;
}