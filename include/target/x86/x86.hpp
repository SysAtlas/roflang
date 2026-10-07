#pragma once
#include <common.hpp>
#include <list>
#include <memory>
#include <vector>

#include <target/x86/registers.hpp>
#include <target/x86/instructions.hpp>
#include <target/x86/basicblock.hpp>

namespace X86 {

class Builder;

enum class LinkageType { Extern, Internal };

struct Function {
  std::string Name;
  LinkageType Linkage;
  std::string toString() const;

  std::list<BasicBlock> BBs;

  BasicBlock &addBasicBlock(BasicBlock &&BB, BasicBlock* InsertAfter);
public:
  Function(std::string_view Name, LinkageType Linkage);
  Function(const Function &) = delete;
  Function(Function&&) = default;
  usize size() { return BBs.size(); }
  friend Builder;
};

struct Module {
  std::string emitHeader();

  std::vector<std::unique_ptr<Function>> Functions;

  Function &addFunction(std::unique_ptr<Function> Fn);

  public:
  std::string toString();
  Module() {}
  friend Builder;
};

} // namespace X86