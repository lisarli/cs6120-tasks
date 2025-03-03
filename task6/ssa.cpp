#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <map>

#include "../task5/dom_utils.hpp"


using PhiVars = std::map<int,std::set<std::string>>; // this type represents a map of blocks to vars for which they have phi-nodes

// get map of blocks to variable names for which they have phi-nodes
PhiVars get_phi_vars(const json& func, const Cfg& cfg, const Dom& dom){
    PhiVars phi_nodes;

    // get names of vars and blocks in which they are assigned
    std::set<std::string> vars;
    std::map<std::string,std::vector<int>> defs;
    for(int i = 0; i < cfg.blocks.size(); i++){
        phi_nodes[i];
        phi_nodes.at(i);
        auto& block = cfg.blocks[i];
        for(auto& instr: block){
            if(instr.contains("dest")){
                auto dest = instr["dest"];
                vars.insert(dest);
                defs[dest].push_back(i);
            }
        }
    }

    // get dominance frontiers
    DomFrontier front = get_dom_frontier(dom, cfg);
    
    // place phi-nodes
    for(auto var: vars){
        auto& cur_defs = defs[var];
        for(int i = 0; i < cur_defs.size(); i++){
            auto d = cur_defs[i];
            for(auto b: front[d]){
                phi_nodes[b].insert(var);
                if(std::find(cur_defs.begin(), cur_defs.end(), b) == cur_defs.end()){
                    cur_defs.push_back(b);
                }
            }
        }
    }

    return phi_nodes;
}

using NameLog = std::map<std::string,std::stack<std::string>>;
using NameCount = std::map<std::string,int>;
using PhiGets = std::map<int,std::map<std::string,std::string>>; // map of block id to map of original var to name to get as
using PhiSets =  std::map<int,std::vector<std::tuple<std::string,std::string,int>>>; // tuples of (original var, what to set it with, block id of block where phi node is located)
using UndefInits = std::map<int,std::vector<std::pair<std::string,std::string>>>; // map of block id to list of (var name, type) which need to be assigned undef at start of block

void rename(int block_id, Cfg& cfg, const DomTree& tree, const PhiVars& phi_vars, NameLog& name_log, NameCount& name_count, PhiGets& phi_gets, PhiSets& phi_sets, UndefInits& undefs, const std::map<std::string,std::string>& var_to_type){    
    // std::cout << "in rename for block: " << block_id << std::endl;
    // initialize undefs and phi gets and sets as empty
    phi_gets[block_id];
    phi_sets[block_id];
    undefs[block_id];

    // rename within current block
    auto& blocks = cfg.blocks;
    std::map<std::string,int> rename_count;

    // rename all phi-node dests
    for(auto var: phi_vars.at(block_id)){
        auto new_name = var + "." + std::to_string(name_count[var]);
        name_count[var]++;
        name_log[var].push(new_name);
        rename_count[var]++;
        phi_gets[block_id][var] = new_name;
    }

    // rename args and dests within current block
    for(auto& instr: blocks[block_id]){
        // process usages (args)
        if(instr.contains("args")){
            for(auto& arg: instr["args"]){
                // read current name
                arg = name_log[arg].top();
            }
        }

        // rename dest
        if(instr.contains("dest")){
            auto dest = instr["dest"];
            instr["dest"] = instr["dest"].get<std::string>() + "." + std::to_string(name_count[dest]);
            name_count[dest]++;
            name_log[dest].push(instr["dest"]);
            rename_count[dest]++;
        }
    }

    // write to successor's phi-nodes by assigning sets in current node
    // std::cout << "starting assign sets" << std::endl;
    for(auto s: cfg.succs.at(block_id)){
        for(auto p: phi_vars.at(s)){
            if(name_log[p].empty()){
                auto new_name = p + "." + std::to_string(block_id) + "." + "init";
                name_log[p].push(new_name);
                rename_count[p]++;
                undefs[block_id].push_back(std::make_pair(new_name, var_to_type.at(p)));
            }
            phi_sets[block_id].push_back(std::make_tuple(p, name_log[p].top(), s));
        }
    }

    // rename children
    for(auto child: tree.at(block_id)){
        rename(child, cfg, tree, phi_vars, name_log, name_count, phi_gets, phi_sets, undefs, var_to_type);
    }

    // pop pushed names
    for(auto [var, count]: rename_count){
        for(int i = 0; i < count; i++){
            name_log[var].pop();
        }
    }
}

bool is_jump(const json& instr){
    return instr.contains("op") && (instr["op"]=="jmp" || instr["op"]=="br");
}

// insert sets, gets, and udefs
std::vector<Block> insert_sets_gets_undefs(const Cfg& cfg, const PhiSets& phi_sets, const PhiGets& phi_gets, const UndefInits& undefs, const std::map<std::string,std::string>& var_to_type){
    auto blocks = cfg.blocks;
    
    for(int i = 0; i < blocks.size(); i++){
        auto& block = blocks[i];

        // insert gets
        for(auto& [var_orig,var_get]: phi_gets.at(i)){
            auto get_instr = json{
                {"dest", var_get},
                {"op", "get"},
                {"type", var_to_type.at(var_orig)}
            };
            auto insert_pos = block.begin();
            if(block.size()>0 && block[0].contains("label")){
                insert_pos++;
            }
            block.insert(insert_pos,get_instr);
        }

        // insert undefs
        for(auto& [var,type]: undefs.at(i)){
            auto undef_instr = json{
                {"dest", var},
                {"op", "undef"},
                {"type", type}
            };
            auto insert_pos = block.begin();
            if(block.size()>0 && block[0].contains("label")){
                insert_pos++;
            }
            block.insert(insert_pos,undef_instr);
        }

        // insert sets
        for(auto& tup: phi_sets.at(i)){
            auto var_orig = std::get<0>(tup);
            auto var_set = std::get<1>(tup);
            auto phi_block = std::get<2>(tup);
            auto set_dest = phi_gets.at(phi_block).at(var_orig);
            auto set_instr = json{
                {"op", "set"},
                {"args", json::array({set_dest, var_set})}
            };
            // FIXME: don't put a set before a label
            auto insert_pos = block.end();
            if(block.size()>0 && is_jump(block[block.size()-1])){
                insert_pos--;
            }
            block.insert(insert_pos, set_instr);
        }
    }

    return blocks;
}

// get map of vars to type
std::map<std::string,std::string> get_var_to_type(const json& func){
    std::map<std::string,std::string> var_to_type;
    for(const auto& instr: func["instrs"]){
        if(instr.contains("type")){
            var_to_type[instr["dest"]] = instr["type"];
        }
    }
    return var_to_type;
}

void to_ssa(json& func){
    // get utils
    Cfg cfg = get_cfg_func(func);
    Dom dom = get_dom(func);
    DomTree tree = get_dom_tree(dom, cfg);

    // get map of blocks to variables for which they need phi-nodes
    auto phi_vars = get_phi_vars(func, cfg, dom);

    // get sets and gets
    NameLog name_log;
    for(auto& arg: func["args"]){
        auto& arg_name = arg["name"];
        name_log[arg_name].push(arg_name);
    }
    NameCount name_count;
    PhiGets phi_gets;
    PhiSets phi_sets;
    UndefInits undefs;
    auto var_to_type = get_var_to_type(func);
    rename(cfg.entryIdx, cfg, tree, phi_vars, name_log, name_count, phi_gets, phi_sets, undefs, var_to_type);

    /*
    std::cout << "--- SETS ---" << std::endl;
    for(auto& [block_id, sets]: phi_sets){
        std::cout << "sets for block " << block_id << ": " << std::endl;
        for(auto& p: sets){
            auto var_orig = std::get<0>(p);
            auto var_set = std::get<1>(p);
            auto phi_block = std::get<2>(p);
            auto set_dest = phi_gets[phi_block][var_orig];
            std::cout << "\t(" << set_dest << ", " << var_set << ")" << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "--- GETS ---" << std::endl;
    for(auto& [block_id, gets]: phi_gets){
        std::cout << "gets for block " << block_id << ": " << std::endl;
        for(auto& [var, rename]: gets){
            std::cout << "\t(" << var << ", " << rename << ")" << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "--- UNDEFS ---" << std::endl;
    for(auto& [block_id, undefs]: undefs){
        std::cout << "undefs for block " << block_id << ": " << std::endl;
        for(auto& [var, type]: undefs){
            std::cout << "\t(" << var << ", " << type << ")" << std::endl;
        }
        std::cout << std::endl;
    }
    */

    // insert sets and gets in blocks
    std::vector<Block> ssa_blocks = insert_sets_gets_undefs(cfg, phi_sets, phi_gets, undefs, var_to_type);

    // reorder blocks if we have an artificial entry block
    std::vector<int> block_order;
    for(int i = 0; i < ssa_blocks.size(); i++){
        block_order.push_back(i);
    }
    if(cfg.entryIdx != 0){
        block_order.insert(block_order.begin(), ssa_blocks.size()-1);
        block_order.pop_back();
    }

    // rewrite func as SSA
    std::vector<json> new_func_body;
    for(int b_id: block_order){
        // std::cout << "inserting block: " << b_id << std::endl;
        auto& b = ssa_blocks[b_id];
        new_func_body.insert(new_func_body.end(), b.begin(), b.end());
    }
    func["instrs"] = new_func_body;
}

int main(int argc, char* argv[]) {
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
        // std::cout << "--- PROCESSING FUNC " << func["name"] << " ---" << std::endl;
        // get and print phi-nodes
        // Cfg cfg = get_cfg_func(func);
        // Dom dom = get_dom(func);
        // auto phi_vars = get_phi_vars(func,cfg,dom);
        // for(auto& [b, vars]: phi_vars){
        //     std::cout << "block " << b << " has phi nodes for vars: ";
        //     for(auto var: vars){
        //         std::cout << var << " ";
        //     }
        //     std::cout << std::endl;
        // }
        // std::cout << std::endl;
        
        to_ssa(func);
    }

    std::cout << j;

    return 0;
}
