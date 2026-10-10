#pragma once

#include <cassert>
#include <unordered_map>
#include <backend/x86/module.hpp>
#include <backend/x86/basicblock.hpp>
#include <backend/x86/function.hpp>

namespace X86 {

class Builder {
  Module *mod;

public:
  Function *cur_function = nullptr;
  BasicBlock *cur_bb = nullptr;

  std::unordered_map<std::string, u32> block_name_counter;

  template <typename T>
  T &addInstruction(T &&instr, BasicBlock *ins_bb = nullptr)
    requires std::derived_from<T, Instruction>
  {
    if (ins_bb) {
      return ins_bb->insertInstruction(std::move(instr));
    }
    return cur_bb->insertInstruction(std::move(instr));
  }

  Function &addFunction(Function &&f);

  // Inserts to end by default
  BasicBlock &addBasicBlock(BasicBlock &&bb, BasicBlock *insert_after = nullptr);

  // Inserts to end by default
  BasicBlock &addBasicBlock(const std::string &name = "bb", BasicBlock *insert_after = nullptr);

  BasicBlock &getBasicBlock(std::string_view name);

  Function &getFunction(std::string_view name);

  BasicBlock &getEndBlock();

  BasicBlock *getNextBlock();

  // addInstruction will add new instructions to this basic block
  void setInsertionPoint(BasicBlock &bb);

  // addBasicBlock will add new basic blocks to this function
  void setBBInsertionPoint(Function &f);

  Builder(Module *mod);
};

} // namespace X86