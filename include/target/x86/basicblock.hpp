#pragma once

#include <string>
#include <list>
#include <memory>
#include <target/x86/instructions.hpp>

namespace X86 {

struct BasicBlock {
  std::string toString() const;
  std::string Name;
  std::list<std::unique_ptr<Instruction>> Instructions;

  BasicBlock(std::string_view Name);

  template <typename T>
  T &insertInstruction(std::unique_ptr<T> Instr)
    requires std::derived_from<T, Instruction>
  {
    Instructions.push_back(std::move(Instr));
    return static_cast<T &>(*Instructions.back());
  }
};

} // namespace X86