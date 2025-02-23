#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Block = std::vector<json>;

struct Cfg {
    std::unordered_map<int,std::vector<int>> preds;
    std::unordered_map<int,std::vector<int>> succs;
    std::vector<Block> blocks;
    int entryIdx;
};

std::vector<Block> get_blocks(const json& func);

Cfg get_cfg(std::vector<Block> bb);

Cfg get_cfg_func(const json& func);

void print_bb(std::vector<Block> bb);

void print_cfg(Cfg cfg);

std::string get_block_name(const Cfg& cfg, int block_idx);