#include <backend/x86/basicblock.hpp>
#include <backend/x86/function.hpp>

namespace X86 {

std::string Function::toString() const {
  std::string res;
  if (rpo.empty()) {
    for (const BasicBlock &bb : bbs) {
      res += bb.toString() + '\n';
    }
  } else {
    for (const BasicBlock* bb : rpo) {
      res += bb->toString() + '\n';
    }
  }
  return res;
}

std::string Function::dbgString() const {
  std::string res;
  if (rpo.empty()) {
    for (const BasicBlock &bb : bbs) {
      res += bb.dbgString() + '\n';
    }
  } else {
    for (const BasicBlock* bb : rpo) {
      res += bb->dbgString() + '\n';
    }
  }
  return res;
}

Function::Function(std::string_view name, Linkage linkage)
    : name{name}, linkage{linkage}, entry_block{nullptr} {}

BasicBlock &Function::addBasicBlock(BasicBlock &&bb, BasicBlock *insert_after) {
  if (insert_after == nullptr) {
    bbs.push_back(std::move(bb));
    return bbs.back();
  }
  return *bbs.insert(
      ++std::ranges::find_if(bbs,
                             [insert_after](const BasicBlock &arg) {
                               return &arg == insert_after;
                             }),
      std::move(bb));
}

Function::Function(Function &&) = default;

usize Function::size() { return bbs.size(); }

Function::~Function() = default;

Function &Function::operator=(Function &&) = default;

} // namespace X86