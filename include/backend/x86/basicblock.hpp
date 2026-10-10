#pragma once

#include <common.hpp>
#include <string>
#include <list>
#include <set>
#include <backend/x86/instructions.hpp>

namespace X86 {

class Instruction;
class Register;

class BasicBlock {
  public:
  std::string toString() const;
  std::string dbgString() const;
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