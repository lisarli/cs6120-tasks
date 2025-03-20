#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <stack>
#include <map>

#include "../task5/dom_utils.hpp"

// FIXME: need to actually update labels for jumps to loops (go to preheader)

// get headers and their backedges in CFG
auto get_headers(const Cfg& cfg, const Dom& dom){
    std::map<int,std::set<int>> headers;
    
    for(int i = 0; i < cfg.blocks.size(); i++){
        for(int succ: cfg.succs.at(i)){
            // check if dominated by successor
            if(dom.at(i).contains(succ)){
                headers[succ].insert(i);
            }
        }
    }
    return headers;
}

// insert loop preheaders
auto insert_preheaders(Cfg& cfg, const std::map<int,std::set<int>>& headers){
    // insert preheaders
    std::map<int,int> h_to_ph;

    int cur_block_id = cfg.blocks.size();
    auto& preds = cfg.preds;
    auto& succs = cfg.succs;
    auto& b_order = cfg.block_order;

    for(auto& [h, backsrc]: headers){
        // update succs for other blocks
        for(int i = 0; i < cfg.blocks.size(); i++){
            if(backsrc.contains(i)) continue;
            auto& cur_succs = succs[i];
            if(cur_succs.contains(h)){
                cur_succs.erase(cur_succs.find(h));
                cur_succs.insert(cur_block_id);
            }
        }
        
        // create preheader block
        h_to_ph[h] = cur_block_id;
        Block empty_block;
        cfg.blocks.push_back(empty_block);
        b_order.insert(std::find(b_order.begin(),b_order.end(),h), cur_block_id);

        // update preds and succs header and preheader
        preds[cur_block_id] = preds[h];
        succs[cur_block_id] = {h};
        preds[h] = {cur_block_id};

        cur_block_id++;
    }

    return h_to_ph;
}

// get blocks in loop body with loop header [h]
auto get_loop_body(int h, std::set<int> backsrc, const Cfg& cfg){
    std::set<int> body;
    body.insert(h);

    for(int b: backsrc){
        std::stack<int> st;
        st.push(b);

        while(!st.empty()){
            int top = st.top(); st.pop();
            if(!body.contains(top)){
                body.insert(top);
                for(int pred: cfg.preds.at(top)){
                    st.push(pred);
                }
            }
        }
    }

    return body;
}

// get maps of names of vars defined in a loop body to where they are defined
auto get_defs_in_body(std::set<int> body, const Cfg& cfg){
    std::map<std::string,std::pair<int,int>> defs;
    for(int b: body){
        auto cur_instrs = cfg.blocks.at(b);
        for(int i = 0; i < cur_instrs.size(); i++){
            auto& instr = cur_instrs.at(i);
            if(instr.contains("dest")){
                // this doesn't work for sets, but we are ignoring those
                defs[instr["dest"]] = std::make_pair(b,i);
            }
        }
    }
    return defs;
}

const std::set<std::string> side_effect_ops = {"set","get","br","div","print","call","store","load"};

// get map of blocks to loop-invariant insns
auto get_loop_inv_instrs(std::set<int> body, const Cfg& cfg){
    std::map<int,std::set<int>> mp;
    std::vector<std::pair<int,int>> inv_instrs;

    auto defs_in_body = get_defs_in_body(body, cfg);
    std::set<std::string> inv_defs;
    auto& blocks = cfg.blocks;

    bool changed = true;
    while(changed){
        auto old_inv_instrs = mp;

        for(int b: body){
            for(int i = 0; i < blocks.at(b).size(); i++){
                auto& instr = blocks.at(b).at(i);
                if(instr.contains("args") && instr.contains("op") && !side_effect_ops.contains(instr["op"])){
                    bool inv = true;

                    // check if instr is loop-invariant
                    for(auto arg: instr["args"]){
                        bool cur_inv = !defs_in_body.contains(arg);
                        if(!inv){
                            auto [b_p,i_p] = defs_in_body[arg];
                            cur_inv = mp.contains(b_p) && mp[b_p].contains(i_p);
                        }
                        inv &= cur_inv;
                    }

                    // mark as loop-invariant
                    if(inv && (!mp.contains(b) || !mp[b].contains(i))){
                        mp[b].insert(i);
                        inv_defs.insert(instr["dest"]);
                        inv_instrs.push_back(std::make_pair(b,i));
                    }
                }
            }
        }

        changed = old_inv_instrs != mp;
    }

    return inv_instrs;
}

// move loop-invariant insns to loop preheader if conditions met
void move_instrs(std::vector<std::pair<int,int>> inv_instrs, int ph, Cfg& cfg){
    auto& blocks = cfg.blocks;
    std::map<int,std::set<int>> mp;
    for(auto& [b, idx]: inv_instrs){
        // TODO: actually check whether safe to move to preheader
        mp[b].insert(idx);

        // insert into preheader
        auto instr = blocks[b][idx];
        blocks[ph].push_back(instr);
    }

    // remove instrs from loop body
    for(auto [b, remove]: mp){
        Block new_block;
        for(int i = 0; i < cfg.blocks[b].size(); i++){
            if(!remove.contains(i)) new_block.push_back(cfg.blocks[b][i]);
        }
        cfg.blocks[b] = new_block;
    }
}

// replace function body of [func] with blocks in [cfg]
void replace_func_body(const Cfg& cfg, json& func){
    std::vector<json> new_func_body;
    for(int b_id: cfg.block_order){
        auto& b = cfg.blocks[b_id];
        new_func_body.insert(new_func_body.end(), b.begin(), b.end());
    }
    func["instrs"] = new_func_body;
}

// rewrite func with LICM
void licm(json& func){
    Cfg cfg = get_cfg_func(func);
    Dom dom = get_dom(func);

    // find backedges
    auto headers = get_headers(cfg, dom);

    // make preheaders
    auto phs = insert_preheaders(cfg, headers);

    // for each loop
    for(auto [h, backsrc]: headers){
        // get loop body
        auto body = get_loop_body(h, backsrc, cfg);
        
        // find loop-invariant insns
        auto inv_instrs = get_loop_inv_instrs(body, cfg);

        // move insns to preheader
        move_instrs(inv_instrs, phs[h], cfg);
    }

    // write new func body
    replace_func_body(cfg, func);
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
        licm(func);
    }

    std::cout << j;

    return 0;
}
