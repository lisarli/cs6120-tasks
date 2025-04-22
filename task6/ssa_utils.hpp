#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <stack>
#include <map>

#include "../task5/dom_utils.hpp"

void to_ssa(json& func);
void from_ssa(json& func);