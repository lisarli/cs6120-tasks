#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <stack>
#include <map>

#include "../task5/dom_utils.hpp"

using FuncArgsMap = std::unordered_map<std::string, std::vector<json>>;

struct Trace {
    std::vector<json> trace;
    std::string lastLabel;
    int lastOffset;
};

// process raw trace file
Trace getTrace(std::string trace_f, FuncArgsMap func_args){
    Trace t;
    auto& trace = t.trace;

    // open trace
    std::ifstream trace_raw(trace_f);
    if (!trace_raw) {
        std::cerr << "ERROR: Could not open trace " << trace_f << std::endl;
        return t;
    }

    std::string line;
    std::stack<std::pair<std::string,std::string>> ret_dest;
    while (std::getline(trace_raw, line)) {
        try {
            json cur_line = nlohmann::json::parse(line);
            if(cur_line.contains("op")){
                // convert branches to guards
                auto cur_op = cur_line["op"];
                if(cur_op == "br"){
                    std::getline(trace_raw, line);

                    std::string cond = cur_line["args"][0];
                    if(line == "false"){
                        // construct negated cond
                        auto neg_cond = json{
                            {"args", {cond}},
                            {"dest", "temp"},
                            {"op", "not"},
                            {"type", "bool"}
                        };
                        trace.push_back(neg_cond);
                        cond = "temp";
                    }

                    // push back guard
                    auto guard = json{
                        {"args", {cond}},
                        {"labels", {"guard_failed"}},
                        {"op", "guard"}
                    };
                    trace.push_back(guard);
                } else if(cur_op == "ret"){
                    auto ret_data = ret_dest.top(); ret_dest.pop();
                    auto ret_id = json{
                        {"args", {cur_line["args"][0]}},
                        {"dest", ret_data.first},
                        {"op", "id"},
                        {"type", ret_data.second}
                    };
                    trace.push_back(ret_id);
                } else if(cur_op == "call"){
                    auto func_name = cur_line["funcs"][0];
                    for(int i = 0; i < cur_line["args"].size(); i++){
                        auto arg = cur_line["args"][i];
                        auto param = func_args[func_name][i];
                        auto arg_id = json{
                            {"args", {arg}},
                            {"dest", param["name"]},
                            {"op", "id"},
                            {"type", param["type"]}
                        };
                        trace.push_back(arg_id);
                    }
                    ret_dest.push(std::make_pair(cur_line["dest"], cur_line["type"]));
                } else if(cur_op != "jmp"){
                    // remove jumps and rets and calls
                    trace.push_back(cur_line);
                }
            }
        } catch (const nlohmann::json::parse_error& e) {
            // end of trace
            t.lastLabel = line;
            std::getline(trace_raw, line);
            t.lastOffset = std::stoi(line);
        }
    }

    // insert speculate, commit, jmp to .trace_success, .guard_failed
    auto speculate = json{
        {"op", "speculate"},
    };
    auto commit = json{
        {"op", "commit"},
    };
    auto jmp_trace_sucess = json{
        {"labels", {"trace_sucess"}},
        {"op", "jmp"},
    };
    auto guard_failed = json{
        {"label", "guard_failed"},
    };
    trace.insert(trace.begin(), speculate);
    trace.push_back(commit);
    trace.push_back(jmp_trace_sucess);
    trace.push_back(guard_failed);

    trace_raw.close();

    return t;
}

// print trace
void printTrace(Trace t){
    std::cout << "\n--- GOT TRACE ---" << std::endl;
    for(auto& cur: t.trace){
        std::cout << cur << std::endl;
    }
    std::cout << "last data: " << t.lastLabel << " " << t.lastOffset << "\n" << std::endl;
}

// insert trace into main
void insertTrace(json& func, Trace t){
    Cfg cfg = get_cfg_func(func);
    auto& blocks = cfg.blocks;
    
    // insert hot path
    auto& entry = blocks[cfg.entryIdx];
    auto& trace = t.trace;
    entry.insert(entry.begin(), trace.begin(), trace.end());

    // insert .trace_success
    for(int i = 0; i < blocks.size(); i++){
        auto& b = blocks[i];
        if(get_block_name(cfg, i) == t.lastLabel){
            auto trace_sucess = json{
                {"label", "trace_sucess"},
            };
            b.insert(b.begin() + t.lastOffset + 1, trace_sucess);
        }
    }
    
    // update body of main
    std::vector<json> new_func_body;
    for(int idx: cfg.block_order){
        new_func_body.insert(new_func_body.end(), blocks[idx].begin(), blocks[idx].end());
    }
    func["instrs"] = new_func_body;
}

int main(int argc, char* argv[]) {
    // get bril program and trace
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << "<file prefix>" << std::endl;
        return 1;
    }
    std::string f = argv[1];
    auto bril_f = f + ".json";
    auto trace_f = f + ".trace"; 

    // open bril json
    json j;
    try {
        std::ifstream input(bril_f);
        if (!input) {
            std::cerr << "ERROR: Could not open file " << bril_f << "\n";
            return 1;
        }
        input >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse Bril JSON from file, " << e.what() << std::endl;
        return 1;
    }

    // get function args
    FuncArgsMap func_args;
    for(auto& func: j["functions"]){
        std::string name = func["name"];
        for(auto arg: func["args"]){
            func_args[name].push_back(arg);
        }
    }

    // process trace
    auto t = getTrace(trace_f, func_args);
    // printTrace(t);
    
    // stitch in trace
    for(auto& func: j["functions"]){
        if(func["name"] == "main"){
            insertTrace(func, t);
        }
    }

    std::cout << j;

    return 0;
}
