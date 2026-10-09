#pragma once

#include <backend/x86/x86.hpp>

// Compute live variables in a function

namespace X86 {

class LiveVariables {
  static void compute(Function &f);
  static void computeCFG(Function &f);
  static void computeCFG(std::unique_ptr<Module> mod);
};

} // namespace X86