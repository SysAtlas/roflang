#pragma once

#include <target/x86/x86.hpp>

namespace X86 {

class Builder {
  Module *M;
  Function* CurFunction;
  BasicBlock *CurBB;

public:
  template <typename T>
  T &insertInstruction(T &&Instr)
    requires std::derived_from<T, InstructionBase>
  {
    return CurBB->insertInstruction(std::make_unique<T>(std::move(Instr)));
  }

  Function& addFunction(Function&& F) {
    CurFunction = &M->addFunction(std::make_unique<Function>(std::move(F)));
    CurBB = &CurFunction->BBs.back();
    return *CurFunction;
  }

  Builder(Module *M) : M{M} {}
};

} // namespace X86