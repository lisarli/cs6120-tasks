#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"


using DomBase = std::unordered_map<int,std::set<int>>;
using Dom = DomBase;
using DomTree = DomBase;
using DomFrontier = DomBase;

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

    return dom;
}

// display nodes and dominators
void print_dom(const DomBase& dom, const Cfg& cfg){
    for(const auto& cur: dom){
        std::cout << get_block_name(cfg, cur.first) << ": ";
        for(auto dominator: cur.second){
            std::cout << get_block_name(cfg, dominator) << " ";
        }
        std::cout << std::endl;
    }
}

/* This method verifies that dominators given by the Dom struct for a node do indeed dominate that node. */
void verify_dominators(const Dom& dom, const Cfg& cfg) {
    for (const auto& [block, dominators] : dom) {
        for (int dominator : dominators) {
            if (dominator == cfg.entryIdx) continue;

            // BFS from entry point, terminate when dominator is found
            std::queue<int> queue(std::queue<int>::container_type(cfg.entryIdx));
            std::set<int> seen{cfg.entryIdx};
            while (!queue.empty()) {
                int current = queue.front(); queue.pop();

                // prune branches if dominator is reached
                if (current == dominator) continue;

                // If block is found then throw an exception
                if (current == block) {
                    throw std::runtime_error(
                        "block "
                        + get_block_name(cfg, dominator)
                        + " is supposed to dominate " 
                        + get_block_name(cfg, block)
                        + " but a path to it from entry was found."
                    );
                }

                for (int succ : cfg.succs.at(current)) {
                    if (seen.find(succ) != seen.end()) continue;
                    seen.insert(succ);
                    queue.push(succ);
                }
            }
        }
    }
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

DomTree get_dom_tree(const Dom& dom, const Cfg& cfg){
    DomTree dom_tree;
    Dom inverse_dom = get_inverse_dom(dom);

    for(const auto& cur: inverse_dom){
        int node = cur.first;
        inverse_dom.at(node).erase(node);
    }

    // for each node find nodes strictly dominated by children of node
    Dom children_dom = inverse_dom;
    for(const auto& cur: children_dom){
        int node = cur.first;
        std::set<int> children;
        for(int child: cur.second){
            std::set<int> temp;
            std::set_union(children.begin(), children.end(),
                           inverse_dom.at(child).begin(), inverse_dom.at(child).end(),
                           std::inserter(temp, temp.end()));
            children = std::move(temp);
        }
        children_dom[node] = children;
    }

    // find domtree, filter nodes
    for(const auto& cur: inverse_dom){
        std::set<int> immediate;
        for(int node: cur.second){
            // if node not in children_dom
            if(!children_dom[cur.first].contains(node)){
                immediate.insert(node);
            }
        }
        dom_tree[cur.first] = immediate;
    }

    return dom_tree;
}

DomFrontier get_dom_frontier(const Dom& dom, const Cfg& cfg){
    DomFrontier front;
    auto inverse_dom = get_inverse_dom(dom);

    for(const auto& cur: inverse_dom){
        int cur_node = cur.first;
        front[cur_node];
        if(!inverse_dom.contains(cur_node)) continue;

        for(int node = 0; node < cfg.blocks.size(); node++){
            // check if not strictly dominated
            if(node==cur_node || !inverse_dom.at(cur_node).contains(node)){
                // check if dominate a predecessor
                for(int pred: cfg.preds.at(node)){
                    if(inverse_dom.at(cur_node).contains(pred)){
                        // insert into dom frontier
                        front[cur_node].insert(node);
                    }
                }
            }
        }
    }

    return front;
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

    const Dom dom = get_dom(func);
    const Cfg cfg = get_cfg_func(func);

    if(utility_type == "dom"){
        print_dom(dom, cfg);
        verify_dominators(dom, cfg);
    } else if(utility_type == "tree"){
        auto tree = get_dom_tree(dom, cfg);
        print_dom(tree, cfg);
    } else if(utility_type == "frontier"){
        auto front = get_dom_frontier(dom, cfg);
        print_dom(front, cfg);
    } else {
        std::cout << "ERROR: Unknown df type, got " << utility_type << std::endl;
        return 1;
    }

    return 0;
}
