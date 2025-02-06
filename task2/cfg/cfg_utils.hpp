#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Block = std::vector<json>;
using Cfg = std::map<int,std::vector<int>>;

std::vector<Block> get_blocks(const json& func);

Cfg get_cfg(std::vector<Block> bb);

void print_bb(std::vector<Block> bb);

void print_cfg(Cfg cfg);