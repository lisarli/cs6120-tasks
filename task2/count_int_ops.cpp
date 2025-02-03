#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


int main() {
    // open bril json
    json j;
    try {
        std::cin >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse JSON from stdin: " << e.what() << std::endl;
        return 1;
    }

    // count arithmetic operations
    std::set<std::string> arith_ops = {"add","sub","mul","div"};
    int num_arith = 0;
    for(const auto& func: j["functions"]){
        for(const auto& instr: func["instrs"]){
            if(instr.contains("op") && arith_ops.count(instr["op"])){
                num_arith++;
            }
        }
    }

    std::cout << "Number of arithmetic operations: " << num_arith << std::endl;

    return 0;
}
