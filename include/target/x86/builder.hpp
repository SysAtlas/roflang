#pragma once

#include <cassert>
#include <target/x86/x86.hpp>
#include <unordered_map>

namespace X86 {

class Builder {
  Module *M;

public:
  Function *CurFunction;
  BasicBlock *CurBB;

  std::unordered_map<std::string, u32> BlockNameCounter{};

  template <typename T>
  T &addInstruction(T &&Instr, BasicBlock *InsBB = nullptr)
    requires std::derived_from<T, Instruction>
  {
    if (InsBB) {
      return InsBB->insertInstruction(std::make_unique<T>(std::move(Instr)));
    }
    return CurBB->insertInstruction(std::make_unique<T>(std::move(Instr)));
  }

  Function &addFunction(Function &&F) {
    return M->addFunction(std::make_unique<Function>(std::move(F)));
  }

  BasicBlock &addBasicBlock(BasicBlock &&BB, BasicBlock* InsertAfter) {
    return CurFunction->addBasicBlock(std::move(BB), InsertAfter);
  }

  BasicBlock &addBasicBlock(const std::string& Name, BasicBlock* InsertAfter) {
    if (BlockNameCounter.find(Name) == BlockNameCounter.end()) {
      BlockNameCounter[Name] = 0;
    }
    return addBasicBlock(BasicBlock(std::format("{}_{}", Name, ++BlockNameCounter[Name])), InsertAfter);
  }

  BasicBlock &getBasicBlock(std::string_view Name) {
    for (BasicBlock &BB : CurFunction->BBs) {
      if (BB.Name == Name) {
        return BB;
      }
    }
    std::cerr << "No such basic block!" << '\n';
    abort();
  }

  BasicBlock &getEndBlock() {
    return getBasicBlock("end");
  }

  BasicBlock *getNextBlock() {
    auto Res = std::ranges::find_if(
      CurFunction->BBs, [this](BasicBlock &BB) { return &BB == CurBB; });
    if (Res == CurFunction->BBs.end()) {
      return nullptr;
    }
    ++Res;
    return (Res != CurFunction->BBs.end()) ? &*Res : nullptr;
  }

  // addInstruction will add new instructions to this basic block
  void setInsertionPoint(BasicBlock &BB) {
    CurBB = &BB;
  }

  // addBasicBlock will add new basic blocks to this function
  void setBBInsertionPoint(Function &F) {
    CurFunction = &F;
  }

  Builder(Module *M) : M{M} {}
};

} // namespace X86