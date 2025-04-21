#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unordered_map>
#include <queue>
#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"

std::pair<std::unordered_map<int, std::set<std::string>>, std::unordered_map<int, std::set<std::string>>> df_live_vars(const json& func);