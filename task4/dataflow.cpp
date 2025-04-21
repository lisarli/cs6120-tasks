#include "dataflow.hpp"

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
std::pair<std::unordered_map<int, T>, std::unordered_map<int, T>>  df_worklist(const json& func, bool is_forward, T init, M merge, R transfer, bool is_display, D display){
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
    if (is_display) {
        display(blocks, in, out);
    }
    return {out, in};
}

using bril_value = std::variant<int, float, bool, char>;
using bril_env = std::unordered_map<std::string, std::optional<bril_value>>;
// key not present = unknown, std::nullopt = non-constant

static void print_bril_env(const bril_env& env, const std::string name) {
    std::cout << "\t" << name << ": { ";
    for (const auto& [var, opt] : env) {
        if (!opt) std::cout << var << ": ?" << "; ";
        else std::visit([var](auto&& val) { std::cout << var << ": " << val << "; "; }, opt.value());
    }
    std::cout << "}" << std::endl;
}

void df_const_propagation(const json& func) {
    bril_env init;

    auto merge = [](bril_env& in_b, const bril_env& out_pred) {
        for (const auto& [var, val] : out_pred) {
            auto it = in_b.find(var);
            if (it == in_b.end()) {     // undefined -> add
                in_b[var] = val;
            } else if (
                it->second == std::nullopt      // non-const
                || it->second.value() != val    // or conflicting values -> invalidate
            ) {
                in_b[var] = std::nullopt;
            }
        }
    };

    auto transfer = [](const Block& block, bril_env& out_b, const bril_env& in_b, int b) {
        bril_env old_out = out_b;
        out_b = in_b;

        for (const auto& inst : block) {
            // consts
            if (inst.contains("value")) {
                std::string type = inst["type"].template get<std::string>();
                if (type == "int") {
                    out_b[inst["dest"].template get<std::string>()] = inst["value"].template get<int>();
                }
                else if (type == "float") {
                    out_b[inst["dest"].template get<std::string>()] = inst["value"].template get<float>();
                }
                else if (type == "bool") {
                    out_b[inst["dest"].template get<std::string>()] = inst["value"].template get<bool>();
                }
                else if (type == "char") {
                    out_b[inst["dest"].template get<std::string>()] = *(inst["value"].template get<std::string>().c_str());
                }
                continue;
            }

            if (!inst.contains("op") || !inst.contains("dest") || !inst.contains("args")) continue;

            std::string op = inst["op"].template get<std::string>();
            std::string dest = inst["dest"].template get<std::string>();
            auto args = inst["args"];
            std::vector<bril_value> values;
            for (auto arg : args) {
                std::string var = arg.template get<std::string>();
                if (out_b.find(var) == out_b.end() || out_b.find(var)->second == std::nullopt) {
                    out_b[dest] = std::nullopt; // dest depends on a non-const; need to invalidate
                    goto next_inst;
                }
                values.push_back((out_b.find(var)->second).value());
            }

            if (op == "add") out_b[dest] = std::get<int>(values[0]) + std::get<int>(values[1]);
            else if (op == "mul") out_b[dest] = std::get<int>(values[0]) * std::get<int>(values[1]);
            else if (op == "sub") out_b[dest] = std::get<int>(values[0]) - std::get<int>(values[1]);
            else if (op == "div") out_b[dest] = std::get<int>(values[0]) / std::get<int>(values[1]);
            else if (op == "eq") out_b[dest] = std::get<int>(values[0]) == std::get<int>(values[1]);
            else if (op == "lt") out_b[dest] = std::get<int>(values[0]) < std::get<int>(values[1]);
            else if (op == "gt") out_b[dest] = std::get<int>(values[0]) > std::get<int>(values[1]);
            else if (op == "le") out_b[dest] = std::get<int>(values[0]) <= std::get<int>(values[1]);
            else if (op == "ge") out_b[dest] = std::get<int>(values[0]) >= std::get<int>(values[1]);

            else if (op == "fadd") out_b[dest] = std::get<float>(values[0]) + std::get<float>(values[1]);
            else if (op == "fmul") out_b[dest] = std::get<float>(values[0]) * std::get<float>(values[1]);
            else if (op == "fsub") out_b[dest] = std::get<float>(values[0]) - std::get<float>(values[1]);
            else if (op == "fdiv") out_b[dest] = std::get<float>(values[0]) / std::get<float>(values[1]);
            else if (op == "feq") out_b[dest] = std::get<float>(values[0]) == std::get<float>(values[1]);
            else if (op == "flt") out_b[dest] = std::get<float>(values[0]) < std::get<float>(values[1]);
            else if (op == "fgt") out_b[dest] = std::get<float>(values[0]) > std::get<float>(values[1]);
            else if (op == "fle") out_b[dest] = std::get<float>(values[0]) <= std::get<float>(values[1]);
            else if (op == "fge") out_b[dest] = std::get<float>(values[0]) >= std::get<float>(values[1]);

            else if (op == "not") out_b[dest] = !std::get<bool>(values[0]);
            else if (op == "and") out_b[dest] = std::get<bool>(values[0]) && std::get<bool>(values[1]);
            else if (op == "or") out_b[dest] = std::get<bool>(values[0]) || std::get<bool>(values[1]);

            else if (op == "ceq") out_b[dest] = std::get<char>(values[0]) == std::get<char>(values[1]);
            else if (op == "clt") out_b[dest] = std::get<char>(values[0]) < std::get<char>(values[1]);
            else if (op == "cgt") out_b[dest] = std::get<char>(values[0]) > std::get<char>(values[1]);
            else if (op == "cle") out_b[dest] = std::get<char>(values[0]) <= std::get<char>(values[1]);
            else if (op == "cge") out_b[dest] = std::get<char>(values[0]) >= std::get<char>(values[1]);
            else if (op == "char2int") out_b[dest] = (int) std::get<char>(values[0]);
            else if (op == "int2char") out_b[dest] = (char) std::get<int>(values[0]);
            
            else if (op == "id") out_b[dest] = values[0];
            else continue;

            next_inst:;
        }

        return old_out != out_b;
    };

    auto display = [](std::vector<Block>& blocks, std::unordered_map<int, bril_env>& in, std::unordered_map<int, bril_env> out) {
        int unlabeled_block_count = 0;
        for (int i = 0; i < blocks.size(); ++i) {
            if (blocks[i].size() == 0) continue;
            std::string label = "b" + std::to_string(unlabeled_block_count);
            if (blocks[i][0].contains("label")) {
                label = blocks[i][0]["label"].template get<std::string>();
            } else {
                ++unlabeled_block_count;
            }
            std::cout << label << std::endl;
            print_bril_env(in[i], "in");
            print_bril_env(out[i], "out");
        }
    };

    df_worklist(func, true, init, merge, transfer, true, display);
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

    df_worklist(func, true, init, merge, transfer, true, display);
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

    df_worklist(func, true, init, merge, transfer, true, display);
}

// live vars df analysis
std::pair<std::unordered_map<int, std::set<std::string>>, std::unordered_map<int, std::set<std::string>>> 
 df_live_vars(const json& func){
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

    return df_worklist(func, false, init, merge, transfer, false, display);
}

int main(int argc, char* argv[]) {
    // get df type
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <defined|live|reaching|constprop>" << std::endl;
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
        } else if (df_type == "constprop") {
            df_const_propagation(func);
        } else{
            std::cout << "ERROR: Unknown df type, got " << df_type << std::endl;
            return 1;
        }
        std::cout << std::endl;
    }

    return 0;
}