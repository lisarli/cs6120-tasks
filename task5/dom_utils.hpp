#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "../task2/cfg/cfg_utils.hpp"


using DomBase = std::unordered_map<int,std::set<int>>;
using Dom = DomBase;
using DomTree = DomBase;
using DomFrontier = DomBase;

// get postorder traversal order
void get_post_order(const Cfg& cfg, std::vector<int>& order, int cur_node, std::set<int>& visited);

// get reversed postorder traversal order
std::vector<int> get_rev_post_order(const Cfg& cfg);

// get map of nodes to dominators
Dom get_dom(const json& func);

// display nodes and dominators
void print_dom(const DomBase& dom, const Cfg& cfg);

/* This method verifies that dominators given by the Dom struct for a node do indeed dominate that node. */
void verify_dominators(const Dom& dom, const Cfg& cfg);

Dom get_inverse_dom(const Dom& dom);

DomTree get_dom_tree(const Dom& dom, const Cfg& cfg);

DomFrontier get_dom_frontier(const Dom& dom, const Cfg& cfg);

Dom find_dominators_brute_force(const Cfg& cfg);