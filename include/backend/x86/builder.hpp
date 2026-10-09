#pragma once

#include <cassert>
#include <unordered_map>
#include <backend/x86/x86.hpp>

namespace X86 {

class Builder {
  Module *mod;

public:
  Function *cur_function = nullptr;
  BasicBlock *cur_bb = nullptr;

  std::unordered_map<std::string, u32> block_name_counter{};

  template <typename T>
  T &addInstruction(T &&instr, BasicBlock *ins_bb = nullptr)
    requires std::derived_from<T, Instruction>
  {
    if (ins_bb) {
      return ins_bb->insertInstruction(std::move(instr));
    }
    return cur_bb->insertInstruction(std::move(instr));
  }

  Function &addFunction(Function &&f) {
    return mod->addFunction(std::make_unique<Function>(std::move(f)));
  }

  BasicBlock &addBasicBlock(BasicBlock &&bb, BasicBlock* insert_after) {
    return cur_function->addBasicBlock(std::move(bb), insert_after);
  }

  BasicBlock &addBasicBlock(const std::string& name, BasicBlock* insert_after) {
    if (block_name_counter.find(name) == block_name_counter.end()) {
      block_name_counter[name] = 0;
    }
    return addBasicBlock(BasicBlock(std::format("{}_{}", name, ++block_name_counter[name])), insert_after);
  }

  BasicBlock &getBasicBlock(std::string_view name) {
    for (BasicBlock &bb : cur_function->bbs) {
      if (bb.name == name) {
        return bb;
      }
    }
    std::cerr << "No such basic block!" << '\n';
    abort();
  }

  Function& getFunction(std::string_view name) {
    for (const auto& f : mod->functions) {
      if (f->name == name) {
        return *f;
      }
    }
    std::cerr << "No such function in the module!" << '\n';
    abort();
  }

  BasicBlock &getEndBlock() {
    return getBasicBlock("end");
  }

  BasicBlock *getNextBlock() {
    auto res = std::ranges::find_if(
      cur_function->bbs, [this](BasicBlock &bb) { return &bb == cur_bb; });
    if (res == cur_function->bbs.end()) {
      return nullptr;
    }
    ++res;
    return (res != cur_function->bbs.end()) ? &*res : nullptr;
  }

  // addInstruction will add new instructions to this basic block
  void setInsertionPoint(BasicBlock &BB) {
    cur_bb = &BB;
  }

  // addBasicBlock will add new basic blocks to this function
  void setBBInsertionPoint(Function &F) {
    cur_function = &F;
  }

  Builder(Module *mod) : mod{mod} {}
};

} // namespace X86