#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unordered_map>
#include <queue>
#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"

using bril_value = std::variant<int, float, bool, char>;
using bril_env = std::unordered_map<std::string, std::optional<bril_value>>;
using DefStore = std::unordered_map<std::string,std::set<std::string>>;

using DFLiveVars = std::pair<std::unordered_map<int, std::set<std::string>>, std::unordered_map<int, std::set<std::string>>>;
using DFDefinedVars = std::pair<std::unordered_map<int, std::set<std::string>>, std::unordered_map<int, std::set<std::string>>>;
using DFConstProp = std::pair<std::unordered_map<int, bril_env>, std::unordered_map<int, bril_env>>;
using DFReachingDefs = std::pair<std::unordered_map<int, DefStore>, std::unordered_map<int, DefStore>>;

DFLiveVars df_live_vars(const json& func, bool is_display);

DFDefinedVars df_defined_vars(const json& func, bool is_display);

DFReachingDefs df_reaching_defs(const json& func, bool is_display);

DFConstProp df_const_propagation(const json& func, bool is_display);