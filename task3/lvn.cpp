#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unordered_map>
#include <format>
#include <cassert>
#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"

const std::set<std::string> known_ops = {"add", "sub", "mul", "div", "eq", "lt", "gt", "le", "ge", "and", "or", "not", "fadd", "fsub", "fmul", "fdiv", "feq", "flt", "fgt", "fle", "fge", "const", "id"};
const std::set<std::string> comm_ops = {"add","mul","eq","and","or","fadd","fmul","feq"};
const std::string id_prefix = "lvnv_";
int id_num = 0;

class Value {
public:
    json canon_instr;

    Value() {}

    Value(json instr, std::unordered_map<std::string,int>& var_to_id){
        // std::cout << "getting canonical for: " << instr << std::endl;
        canon_instr = instr;
        canon_instr["dest"] = "";

        // make canonical representation if commutative
        auto& args = canon_instr["args"];
        if(comm_ops.count(instr["op"])){
            std::sort(args.begin(),args.end());
        }

        for(auto& arg: args){
            if(!var_to_id.count(arg)){
                // std::cout << arg << "missing" << std::endl;
            }
            assert(var_to_id.count(arg));
            arg = "id_" + std::to_string(var_to_id[arg]);
        }
        // std::cout << "got canonical: " << canon_instr << std::endl;
    }

    bool operator==(const Value& other) const {
        return canon_instr == other.canon_instr;
    }
};

template <>
struct std::hash<Value>
{
  std::size_t operator()(const Value& v) const
  {
    return std::hash<json>{}(v.canon_instr);
  }
};

std::vector<bool> get_last_def(Block& b){
    std::vector<bool> last_def(b.size());
    std::set<std::string> seen;
    for(int i = b.size()-1; i >= 0; i--){
        auto& instr = b[i];
        if(instr.contains("dest")){
            auto dest = instr["dest"];
            if(seen.count(dest)){
            last_def[i] = false;
            } else{
                last_def[i] = true;
            }
            seen.insert(dest);
        }
    }
    return last_def;
}


void lvn_block(Block& b){
    std::unordered_map<Value,int> table_new; // value to id
    std::unordered_map<std::string,int> var_to_id; // program var
    std::unordered_map<int,std::string> id_to_var; // id to program var

    auto last_def = get_last_def(b);
    for(int i = 0; i < b.size(); i++){
        auto& instr = b[i];
        // std::cout << "checking instr: " << instr << std::endl;

        // update args according to var_to_id
        // std::cout << "updating args" << std::endl;
        bool first_live_in = false;
        if(instr.contains("args")){
            for(auto& var: instr["args"]){
                // std::cout << "searching for " << var << ", contains is " << var_to_id.count(var) << std::endl;
                if(var_to_id.count(var)){
                    // std::cout << "found var, maps to " << var_to_id[var] << ", which maps to " << id_to_var[var_to_id[var]] << std::endl;
                    var = id_to_var[var_to_id[var]];
                } else{
                    // live-in
                    // std::cout << "inserting live in with id " << id_num << ", currently in var " << var << std::endl;
                    first_live_in = true;
                    var_to_id[var] = id_num;
                    id_to_var[id_num] = var;
                    id_num++;
                }
            }
        }
        // std::cout << "updated args: " << instr << std::endl;

        Value v;
        bool has_val = instr.contains("op") && known_ops.count(instr["op"]);
        bool new_val = true;
        int dest_id = -1;

        if(has_val){
            // std::cout << "is has val op" << std::endl;
            v = Value(instr, var_to_id);
            // std::cout << "checking for value: " << v.canon_instr << std::endl;
            if(table_new.count(v)){
                new_val = false;
                // std::cout << "found in table" << std::endl;
                // replace instruction with id if value exists
                dest_id = table_new[v];

                auto dest = instr["dest"];
                // make dead code to be cleaned up
                b[i] = json{
                    {"dest", dest},
                    {"op", "id"},
                    {"type", instr["type"]},
                    {"args", {id_to_var[dest_id]}}
                };
                var_to_id[dest] = dest_id;
            }
        }

        if(instr.contains("dest")){
            // create new id
            std::string dest = instr["dest"];
            std::string lvnv_name = dest;
            if(!last_def[i]){
                lvnv_name = id_prefix + std::to_string(id_num);
                id_num++;
                instr["dest"] = lvnv_name;
            }
            if(new_val){
                dest_id = id_num;
                id_num++;
                table_new[v] = dest_id;
                var_to_id[lvnv_name] = dest_id;
                id_to_var[dest_id] = lvnv_name;
                // std::cout << "mapped new value " << v.canon_instr << " to " << dest_id << " in table" << std::endl;
            }
            // copy propagation
            if(instr["op"]=="id" && !first_live_in){
                dest_id = var_to_id[instr["args"][0]];
            }
            // std::cout << "mapping var " << dest << " to " << dest_id << ", which maps to " << id_to_var[dest_id] << std::endl;
            var_to_id[dest] = dest_id;
        }
        // std::cout << "updated instr: " << instr << std::endl;
        // std::cout << "iter done" << std::endl << std::endl;

    }
}

void lvn(json& func){
    std::vector<Block> blocks = get_blocks(func);
    for(auto& b: blocks){
        lvn_block(b);
    }

    // update function body
    std::vector<json> new_func_body;
    for(const auto& b: blocks){
        new_func_body.insert(new_func_body.end(), b.begin(), b.end());
    }
    func["instrs"] = new_func_body;
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
    
    // rename with lvn 
    for(auto& func: j["functions"]){
        lvn(func);
    }

    // output new program
    std::cout << j << std::endl;

    return 0;
}
