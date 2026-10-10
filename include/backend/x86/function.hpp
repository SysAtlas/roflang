#pragma once

#include <string>
#include <list>
#include <common.hpp>
#include <unordered_map>
#include <backend/x86/livevariables.hpp>

namespace X86 {

class BasicBlock;
enum class Linkage;
class Builder;

class Function {
  public:
  std::string name;
  Linkage linkage;
  std::string toString() const;
  std::string dbgString() const;

  std::list<BasicBlock> bbs;
  BasicBlock* entry_block;

  std::list<BasicBlock*> rpo;
  std::unordered_map<u32, LiveInterval> live_intervals;

  // Basic block is inserted after last basic block in bbs if insert_after is nullptr
  BasicBlock &addBasicBlock(BasicBlock &&bb, BasicBlock *insert_after);

public:
  Function(std::string_view name, Linkage linkage);

  Function(const Function &) = delete;
  Function operator=(const Function&) = delete;
  Function(Function &&);
  ~Function();
  Function& operator=(Function&&);
  usize size();
  friend Builder;
};

} // namespace X86