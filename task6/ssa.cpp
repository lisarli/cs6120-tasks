#include "ssa_utils.hpp"

int main(int argc, char* argv[]) {
    // get utility type
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << "<to|from>" << std::endl;
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
    for(auto& func: j["functions"]){
        if (utility_type == "to") {
            to_ssa(func);
        } else if (utility_type == "from") {
            from_ssa(func);
        } else {
            std::cerr << "ERROR: Unknown utility type, got " << utility_type << std::endl;
            return 1;
        }
    }

    std::cout << j;

    return 0;
}
