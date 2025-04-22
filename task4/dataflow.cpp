#include "dataflow_utils.hpp"

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
            df_defined_vars(func, true);
        } else if(df_type == "live"){
            df_live_vars(func, true);
        } else if(df_type == "reaching"){
            df_reaching_defs(func, true);
        } else if (df_type == "constprop") {
            df_const_propagation(func, true);
        } else{
            std::cout << "ERROR: Unknown df type, got " << df_type << std::endl;
            return 1;
        }
        std::cout << std::endl;
    }

    return 0;
}