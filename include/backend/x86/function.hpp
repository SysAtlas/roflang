#pragma once

#include <string>
#include <list>
#include <common.hpp>

namespace X86 {

struct BasicBlock;
enum class Linkage;
class Builder;

struct Function {
  std::string name;
  Linkage linkage;
  std::string toString() const;

  std::list<BasicBlock> bbs;
  BasicBlock* entry_block;

  std::list<BasicBlock*> rpo;

  BasicBlock &addBasicBlock(BasicBlock &&bb, BasicBlock *insert_after);

public:
  Function(std::string_view name, Linkage linkage);
  Function(const Function &) = delete;
  Function(Function &&) = default;
  usize size() { return bbs.size(); }
  friend Builder;
};

} // namespace X86