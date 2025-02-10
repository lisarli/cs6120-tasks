#include "cfg_utils.hpp"


int main() {
    // open bril json
    json j;
    try {
        std::cin >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse JSON from stdin: " << e.what() << std::endl;
        return 1;
    }
    
    for(const auto& func: j["functions"]){
        std::cout << "function: " << func["name"] << std::endl;
        if(func.contains("args")){
            std::cout << "\targs: " << func["args"] << std::endl;
        }

        auto bb = get_blocks(func);
        print_bb(bb);
        auto cfg = get_cfg(bb);
        print_cfg(cfg);
    }

    return 0;
}
