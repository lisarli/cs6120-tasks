#include "dom_utils.hpp"

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
    for(auto& func: j["functions"]){
        const Dom dom = get_dom(func);
        const Cfg cfg = get_cfg_func(func);
        verify_dominators(dom, cfg);
        const Dom dom_brute_force = find_dominators_brute_force(cfg);

        if (dom != dom_brute_force) {
            throw std::runtime_error("Dominator sets don't match up.");
        }

        if(utility_type == "dom"){
            print_dom(dom, cfg);
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
    }

    return 0;
}
