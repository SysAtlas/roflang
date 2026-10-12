#pragma once

#include <vector>
#include <common.hpp>

namespace IR {

class Function;

class Module {
  std::vector<UP<Function>> functions;
};

}