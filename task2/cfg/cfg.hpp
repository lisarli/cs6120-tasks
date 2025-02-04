#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<std::vector<json>> get_blocks(const json& func);

std::map<int,std::vector<int>> get_cfg(std::vector<std::vector<json>> bb);

void print_bb(std::vector<std::vector<json>> bb);

void print_cfg(std::map<int,std::vector<int>> cfg);