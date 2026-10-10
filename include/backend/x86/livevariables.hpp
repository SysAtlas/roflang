#pragma once

#include "backend/x86/registers.hpp"
#include <common.hpp>
#include <list>
#include <set>
#include <vector>

// Compute live variables in a function

namespace X86 {

class Module;
class Function;
class BasicBlock;

// Inclusive on both sides [start;end]
struct LiveInterval {
  u32 start = std::numeric_limits<u32>::max();
  u32 end = 0;
  u32 variable_id = std::numeric_limits<u32>::max();
  // The list of registers the live range can't be assigned to
  std::list<Register> banned_phys;

  // Used by regalloc
  std::optional<Register> alloc_reg;

  std::string toString();
  LiveInterval(u32 start, u32 end, u32 variable_id)
      : start{start}, end{end}, variable_id(variable_id) {}
  LiveInterval(u32 variable_id) : variable_id(variable_id) {}
};

class LiveVariableAnalysis {
private:
  std::set<u32> compute_live_ins(BasicBlock *bb);
  void computeLiveIntervals(Function &f);

  // Required for computing live intervals
  static constexpr u8 ENUMERATION_GAP = 4;

  // Reenumerates instructions and computes clobber sets
  std::vector<std::pair<u32, std::list<Register>>> clobber_sets;
  void computeClobberSets(Function &f);
  void reenumerate(Function &f);
  void dfs(Function &f, BasicBlock &cur, std::set<BasicBlock *> &visited);
  // Required for enumerating instructions
  void reorderBBs(Function &f);

  std::set<u32> computeLiveOut(BasicBlock &bb);
  void computeLiveness(Function &f);

  // Required for solving data flow equations
  void computeCFG(Function &f);

public:
  static void runPass(Module *mod);
};

} // namespace X86