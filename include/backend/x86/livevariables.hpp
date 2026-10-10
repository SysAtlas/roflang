#pragma once

#include <common.hpp>
#include <memory>
#include <set>

// Compute live variables in a function

namespace X86 {

class Module;
class Function;
class BasicBlock;

struct LiveInterval {
  u32 start = std::numeric_limits<u32>::max();
  u32 end = 0;
  std::string toString() { return std::format("[{};{}]", start, end); }
};

class LiveVariables {
private:
  static std::set<u32> compute_live_ins(BasicBlock *bb);
  static void compute_live_intervals(Function &f);

  // Required for computing live intervals
  static constexpr u8 ENUMERATION_GAP = 4;

  static void reenumerate(Function &f);
  static void dfs(Function &f, BasicBlock &cur,
                  std::set<BasicBlock *> &visited);
  // Required for enumerating instructions
  static void reorderBBs(Function &f);

  static std::set<u32> computeLiveOut(BasicBlock &bb);
  static void computeLiveness(Function &f);

  // Required for solving data flow equations
  static void computeCFG(Function &f);
  static void runPipeline(Module *mod);
};

} // namespace X86