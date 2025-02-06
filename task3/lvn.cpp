#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unordered_map>
#include <format>
#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"

const std::set<std::string> known_ops = {"add", "sub", "mul", "div", "eq", "lt", "gt", "le", "ge", "and", "or", "not", "fadd", "fsub", "fmul", "fdiv", "feq", "flt", "fgt", "fle", "fge", "const"};
const std::string id_prefix = "lvnv_";
int id_num = 0;

class Value {
public:
    json instr;

    Value() {}

    Value(json instr){
        instr["dest"] = "";
        this->instr = instr;
    }

    bool operator==(const Value& other) const {
        if(instr["op"] == "add" || instr["op"] == "mul"){
            // TODO: commutativity
        }
        return instr == other.instr;
    }
};

template <>
struct std::hash<Value>
{
  std::size_t operator()(const Value& v) const
  {
    return std::hash<json>{}(v.instr);
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
    std::unordered_map<Value,std::string> table; // value to id
    std::unordered_map<std::string,std::string> var_to_id; // program var to unique id
    auto last_def = get_last_def(b);

    for(int i = 0; i < b.size(); i++){
        auto& instr = b[i];
        // std::cout << "checking instr: " << instr << std::endl;

        // update args according to var_to_id
        // std::cout << "updating args" << std::endl;
        if(instr.contains("args")){
            for(auto& var: instr["args"]){
                // std::cout << "searching for " << var << ", maps to " << var_to_id[var] << std::endl;
                if(var_to_id.count(var)){
                    var = var_to_id[var];
                } else{
                    var_to_id[var] = var; // live-in
                }
            }
        }
        // std::cout << "updated args: " << instr << std::endl;

        Value v;
        bool has_val = instr.contains("op") && known_ops.count(instr["op"]);
        bool new_val = true;
        std::string id;
        if(instr.contains("dest")){ id = instr["dest"]; }

        if(has_val){
            // std::cout << "is has val op" << std::endl;
            v = Value(instr);
            if(table.count(v)){
                new_val = false;
                // std::cout << "found in table" << std::endl;
                // replace instruction with id if value exists
                id = table[v];
                auto dest = instr["dest"];
                // make dead code to be cleaned up
                b[i] = json{
                    {"dest", dest},
                    {"op", "id"},
                    {"type", instr["type"]},
                    {"args", {id}}
                };
                var_to_id[dest] = id;
            }
        }

        if(instr.contains("dest")){
            // create new id
            std::string dest = instr["dest"];
            if(!last_def[i]){
                id = id_prefix + std::to_string(id_num);
                id_num++;
                instr["dest"] = id;
            }
            if(has_val && new_val){
                table[v] = id;
            }
            var_to_id[dest] = id;
            // std::cout << "mapped to " << id << " in table" << std::endl;
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
