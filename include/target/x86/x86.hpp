#pragma once
#include <common.hpp>
#include <list>
#include <memory>
#include <vector>

#include <target/x86/registers.hpp>
#include <target/x86/instructions.hpp>

namespace X86 {

class Builder;

struct BasicBlock {
  std::string toString() const;
  std::string Name;
  std::list<std::unique_ptr<InstructionBase>> Instructions;

public:
  BasicBlock(std::string_view Name);

  template<typename T>
  T &insertInstruction(std::unique_ptr<T> Instr) requires std::derived_from<T, InstructionBase> {
    Instructions.push_back(std::move(Instr));
    return static_cast<T&>(*Instructions.back());
  }
};

enum class LinkageType { Extern, Internal };

struct Function {
  std::string Name;
  LinkageType Linkage;
  std::string toString() const;

  std::list<BasicBlock> BBs;

public:
  Function(std::string_view Name, LinkageType Linkage);
  Function(const Function &) = delete;
  Function(Function&&) = default;
  friend Builder;
};

struct Module {
  std::string emitHeader();

  std::vector<std::unique_ptr<Function>> Functions;

  Function &addFunction(std::unique_ptr<Function> Fn);

  std::string toString();
  Module() {}
  friend Builder;
};

} // namespace X86