#pragma once

#include "common.hpp"
#include <string>
#include <list>
#include <set>

namespace X86 {

class Instruction;
struct Register;

struct BasicBlock {
  std::string toString() const;
  std::string name;
  std::list<Instruction> instructions;

  // CFG analysis, should be factored out of here later probably
  std::set<BasicBlock*> succs;
  std::set<BasicBlock*> preds;

  // Live analysis
  std::set<u32> live_out;
  std::set<u32> ue_var;
  std::set<u32> var_kill;

  BasicBlock(std::string_view name);

  template <typename T>
  T &insertInstruction(T&& instr)
    requires std::derived_from<T, Instruction>
  {
    instructions.emplace_back(instr);
    return static_cast<T &>(instructions.back());
  }
};

} // namespace X86